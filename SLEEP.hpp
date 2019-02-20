class Sleep {
  public:
    Sleep() {

    }

    void Begin() {
      esp_sleep_enable_timer_wakeup(this->sleepTime*1000);
    }

    uint8_t GetWakeupReason() {
      esp_sleep_wakeup_cause_t r = esp_sleep_get_wakeup_cause();
      if(r == ESP_SLEEP_WAKEUP_TIMER) {
        return 1;
      }
      return 0;
    }

    void EnableSleep() {
      this->enableSleep = true;
    }

    void TimerSleep() {
      if(this->enableSleep) {
        esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
        esp_deep_sleep_start();
      } else {
        delay(this->sleepTime);
      }
    }
  private:
    bool enableSleep = false;
    const uint64_t sleepTime = 5000;
};