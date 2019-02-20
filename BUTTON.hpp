#include <driver/rtc_io.h>

template<int pin_regulator_enable, int pin_button>
class Button {
  public:
    Button() {
      this->SetupIO();
      this->SetWakeup();
    }
    float readButton() {
      return analogRead(pin_button) / 4095;
    }

  private:
    void SetupIO() {
      rtc_gpio_init((gpio_num_t)pin_regulator_enable);
      rtc_gpio_init((gpio_num_t)pin_button);

      rtc_gpio_set_direction((gpio_num_t)pin_regulator_enable, RTC_GPIO_MODE_OUTPUT_ONLY);
      rtc_gpio_set_direction((gpio_num_t)pin_button, RTC_GPIO_MODE_INPUT_ONLY);

      rtc_gpio_pulldown_en((gpio_num_t)pin_button);
      rtc_gpio_set_level((gpio_num_t)pin_regulator_enable, 1);
    }
    void SetWakeup() {

    }
};
