#include <driver/rtc_io.h>

template<int pin_regulator_enable, int pin_button>
class Sleep {
  public:
    Sleep(ledclass * ledclass) {
      this->led = ledclass;
      this->SetupHoldPower();
      esp_sleep_enable_timer_wakeup(this->sleepTime * 1000);
    }

    uint8_t GetWakeupReason() {
      esp_sleep_wakeup_cause_t r = esp_sleep_get_wakeup_cause();
      if(r == ESP_SLEEP_WAKEUP_TIMER || r == ESP_SLEEP_WAKEUP_EXT0) {
        return 1;
      }
      return 0;
    }

    void AttachInterrupt(void (* intr) (void *), void * args) {
      gpio_set_direction((gpio_num_t)pin_button, GPIO_MODE_INPUT);
      gpio_pulldown_en((gpio_num_t)pin_button);

      gpio_set_intr_type((gpio_num_t)pin_button, GPIO_INTR_POSEDGE);
      gpio_install_isr_service(ESP_INTR_FLAG_EDGE);
      gpio_isr_handler_add((gpio_num_t)pin_button, (*intr), args);
    }

    void DetachInterrupt() {
      gpio_isr_handler_remove((gpio_num_t)pin_button);
      gpio_reset_pin((gpio_num_t)pin_button);
    }

    void EnableSleep() {
      this->DetachInterrupt();
      this->SetupWakeup();
      this->enableSleep = true;
    }

    void TimerSleep() {
      if(this->enableSleep) {
        esp_deep_sleep_start();
      } else {
        delay(this->sleepTime);
      }
    }

    bool ButtonPressed() {
      return rtc_gpio_get_level((gpio_num_t)pin_button) == 1;
    }

    uint8_t GetButtonMode() {
      if(rtc_gpio_get_level((gpio_num_t)pin_button) == 0) {
        return 0; //Do Nothing, Press Button was to short
      }
      this->led->Color(this->led->WHITE);
      for(uint8_t i = 0; i<50; i++) {
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

    void Shutdown() {
      this->led->Color(this->led->WHITE);
      rtc_gpio_set_level((gpio_num_t)pin_regulator_enable, 0);
      delay(60000);
    }
  private:
    bool enableSleep = false;
    const uint64_t sleepTime = 5000;
    ledclass * led;

    void SetupHoldPower() {
      rtc_gpio_init((gpio_num_t)pin_regulator_enable);
      rtc_gpio_set_direction((gpio_num_t)pin_regulator_enable, RTC_GPIO_MODE_OUTPUT_ONLY);
      rtc_gpio_set_level((gpio_num_t)pin_regulator_enable, 1);
    }

    void SetupWakeup() {
      rtc_gpio_init((gpio_num_t)pin_button);
      rtc_gpio_set_direction((gpio_num_t)pin_button, RTC_GPIO_MODE_INPUT_ONLY);
      rtc_gpio_pulldown_en((gpio_num_t)pin_button);
      rtc_gpio_wakeup_enable((gpio_num_t)pin_button, GPIO_INTR_HIGH_LEVEL);
      esp_sleep_enable_ext0_wakeup((gpio_num_t)pin_button, 1);
      esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    }
};

/*static void IRAM_ATTR InterruptHandler(void * arg) {
if(rtc_gpio_get_level((gpio_num_t)pin_button) == 0) {
return; //Do Nothing, Press Button was to short
}
delay(5000); //Wait 5s
if(rtc_gpio_get_level((gpio_num_t)pin_button) == 0) {
return; //Send Lora-Warn!
}
if(rtc_gpio_get_level((gpio_num_t)pin_button) == 1) {
rtc_gpio_set_level((gpio_num_t)pin_regulator_enable, 0);
return; //Shutdown Controller
}
}*/