#include <driver/rtc_io.h>
#include <mutex>
#include <pthread.h>

template<int pin_regulator_enable, int pin_button>
class Sleep {
  public:
    Sleep(ledclass * ledclass) {
      this->led = ledclass;
      this->SetupHoldPower();
    }

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

    void AttachInterrupt(void (* intr) (void *), void * args) {
      if(pin_button != 0) {
        gpio_set_direction((gpio_num_t)pin_button, GPIO_MODE_INPUT);
        gpio_pulldown_en((gpio_num_t)pin_button);
        gpio_set_intr_type((gpio_num_t)pin_button, GPIO_INTR_POSEDGE);
        gpio_install_isr_service(ESP_INTR_FLAG_EDGE);
        gpio_isr_handler_add((gpio_num_t)pin_button, (*intr), args);
      }
    }

    void EnableSleep() {
      this->enableSleep = true;
    }

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

    uint8_t GetButtonMode() {
      if(pin_button != 0) {
        if(rtc_gpio_get_level((gpio_num_t)pin_button) == 0) {
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
          if(rtc_gpio_get_level((gpio_num_t)pin_button) == 0) {
            this->led->Color(this->led->BLACK);
            return 1; //Send Lora-Warn!
          }
        }
        this->led->Color(this->led->BLACK);
        if(rtc_gpio_get_level((gpio_num_t)pin_button) == 1) {
          return 2; //Shutdown Controller
        }
      }
      return 0;
    }

    void Shutdown() {
      if(pin_regulator_enable != 0) { //Only possible if hardware can shutdown itselfs
        this->led->Color(this->led->WHITE);
        rtc_gpio_set_level((gpio_num_t)pin_regulator_enable, 0);
        delay(60000);
      }
    }

    pthread_mutex_t MutexSleep;
  private:
    bool enableSleep = false;
    bool wakeupByButton = false;
    const uint64_t sleepTime = 5000;
    ledclass * led;

    void SetupHoldPower() {
      if(pin_regulator_enable != 0) {
        rtc_gpio_init((gpio_num_t)pin_regulator_enable);
        rtc_gpio_set_direction((gpio_num_t)pin_regulator_enable, RTC_GPIO_MODE_OUTPUT_ONLY);
        rtc_gpio_set_level((gpio_num_t)pin_regulator_enable, 1);
      }
    }

    void DetachInterrupt() {
      if(pin_button != 0) {
        gpio_isr_handler_remove((gpio_num_t)pin_button);
        gpio_reset_pin((gpio_num_t)pin_button);
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