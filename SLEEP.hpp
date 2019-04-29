#include <driver/rtc_io.h>
#include <mutex>
#include <pthread.h>

/// <summary>
/// Class to enable Sleepmodes and Wakeup
/// </summary>
/// <typeparam name="pin_regulator_enable">Pin number of Pin to hold the EN-Pin from the Powerregulator high, zero to diable function</typeparam>
/// <typeparam name="pin_button">Pin number of the Button-Pin, zero disables button</typeparam>
template<int pin_regulator_enable, int pin_button>
class Sleep {
  public:
    /// <summary>Mutex for not going to sleep, when parsing Button</summary>
    pthread_mutex_t MutexSleep;

    /// <summary>Constructor for Sleep class, setup the io pins</summary>
    Sleep(ledclass * ledclass) {
      this->led = ledclass;
      this->SetupPins();
    }

    /// <summary>Readout the reason why the ESP starts.</summary>
    /// <returns>Returns 1 if the ESP waked up from Interrupt or by Timer, 0 if the ESP was powered on</returns>
    uint8_t GetWakeupReason() {
      esp_sleep_wakeup_cause_t r = esp_sleep_get_wakeup_cause();
      if(r == ESP_SLEEP_WAKEUP_TIMER || r == ESP_SLEEP_WAKEUP_EXT0) {
        if(r == ESP_SLEEP_WAKEUP_EXT0) {
          this->wakeupByButton = true;
        }
        return 1;
      }
      return 0;
    }

    /// <summary>Attatch a function that is called, if the button is pressed</summary>
    /// <typeparam name="intr">Pointer to a static void funct(void * args) function</typeparam>
    /// <typeparam name="args">Pointer to an object that is the first parameter to the function</typeparam>
    void AttachInterrupt(void (* intr) (void *), void * args) {
      if(pin_button != 0) {
        gpio_install_isr_service(ESP_INTR_FLAG_EDGE);
        gpio_isr_handler_add((gpio_num_t)pin_button, (*intr), args);
      }
    }

    /// <summary>Call to activate deepsleep mode</summary>
    void EnableSleep() {
      this->enableSleep = true;
    }

    /// <summary>Call to make a Sleep, depends on calling of EnableSleep if deepsleep mode or delay is used</summary>
    void TimerSleep() {
      if(this->enableSleep) {
        this->DetachInterrupt();
        this->SetupWakeup();
        pthread_mutex_lock(&this->MutexSleep);
        esp_deep_sleep_start();
        pthread_mutex_unlock(&this->MutexSleep);
      } else {
        delay(this->sleepTime);
      }
    }

    /// <summary>Readout how long is the Button pressed</summary>
    /// <returns>Return 0 if button was not pressed, 1 if shorter than 5s, and 2 if longer than 5s</returns>
    uint8_t GetButtonMode() {
      if(pin_button != 0) {
        if(gpio_get_level((gpio_num_t)pin_button) == 0) {
          if(this->wakeupByButton) {
            this->wakeupByButton = false;
            return 1;
          }
          return 0; //Do Nothing, Press Button was to short
        }
        this->led->Color(this->led->WHITE);
        for(uint8_t i = 0; i < 50; i++) {
          delay(100); //Wait 50 * 100ms = 5s
          this->led->Color((i % 2 == 0) ? this->led->BLACK : this->led->WHITE);
          if(gpio_get_level((gpio_num_t)pin_button) == 0) {
            this->led->Color(this->led->BLACK);
            return 1; //Send Lora-Warn!
          }
        }
        this->led->Color(this->led->WHITE);
        if(gpio_get_level((gpio_num_t)pin_button) == 1) {
          return 2; //Shutdown Controller
        }
      }
      return 0;
    }

    /// <summary>Call to Shutdown the device</summary>
    void Shutdown() {
      if(pin_regulator_enable != 0) { //Only possible if hardware can shutdown itselfs
        this->led->Color(this->led->WHITE);
        rtc_gpio_set_level((gpio_num_t)pin_regulator_enable, 0);
        delay(60000);
      }
      this->led->Color(this->led->BLACK);
    }

    ///<summary>Wait a Random Time (0, 350 or 700ms)</summary>
    void WaitRandom() {
      uint8_t waiting = random(0, 2);
      if(waiting == 1) {
        delay(350);
      } else if(waiting == 2) {
        delay(700);
      }
    }
    
    ///<summary>Set three Bytes as Entropy for PRNG</summary>
    /// <typeparam name="r1">first byte entropy</typeparam>
    /// <typeparam name="r2">second byte entropy</typeparam>
    /// <typeparam name="r3">third byte entropy</typeparam>
    void SetEntropy(uint8_t r1, uint8_t r2, uint8_t r3) {
      uint8_t rand[] = { r1, r2, r3 };
      esp_fill_random(rand, 3);
    }
  private:
    bool enableSleep = false;
    bool wakeupByButton = false;
    const uint64_t sleepTime = 5000;
    ledclass * led;

    void SetupPins() {
      if(pin_regulator_enable != 0) {
        rtc_gpio_init((gpio_num_t)pin_regulator_enable);
        rtc_gpio_set_direction((gpio_num_t)pin_regulator_enable, RTC_GPIO_MODE_OUTPUT_ONLY);
        rtc_gpio_set_level((gpio_num_t)pin_regulator_enable, 1);
      }
      if(pin_button != 0) {
        gpio_config_t io_conf;
        io_conf.pin_bit_mask = (1ULL << pin_button);
        io_conf.mode = GPIO_MODE_INPUT;
        io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
        io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
        io_conf.intr_type = GPIO_INTR_POSEDGE;
        gpio_config(&io_conf);
      }
    }

    void DetachInterrupt() {
      if(pin_button != 0) {
        gpio_isr_handler_remove((gpio_num_t)pin_button);
      }
    }

    void SetupWakeup() {
      esp_sleep_enable_timer_wakeup(this->sleepTime * 1000);
      if(pin_button != 0) {
        rtc_gpio_init((gpio_num_t)pin_button);
        rtc_gpio_set_direction((gpio_num_t)pin_button, RTC_GPIO_MODE_INPUT_ONLY);
        rtc_gpio_pulldown_en((gpio_num_t)pin_button);
        rtc_gpio_wakeup_enable((gpio_num_t)pin_button, GPIO_INTR_HIGH_LEVEL);
        esp_sleep_enable_ext0_wakeup((gpio_num_t)pin_button, 1);
        esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
      }
    }
};