#include <driver/rtc_io.h>

template<int pin_regulator_enable, int pin_button>
class Button {
  public:
    Button() {
      this->SetupIO();
      this->SetWakeup();
    }
	
	bool Pressed() {
	  return rtc_gpio_get_level((gpio_num_t)pin_button) == 1;
	}

	uint8_t GetTask() {
	  if(rtc_gpio_get_level((gpio_num_t)pin_button) == 0) {
        return 0; //Do Nothing, Press Button was to short
      }
	  for(uint8_t i=0;i<50;i++) {
		delay(100); //Wait 50 * 100ms = 5s
        if(rtc_gpio_get_level((gpio_num_t)pin_button) == 0) {
          return 1; //Send Lora-Warn!
        } 
	  }
	  if(rtc_gpio_get_level((gpio_num_t)pin_button) == 1) {
        return 2; //Shutdown Controller
      }
	}
	
	void Shutdown() {
	  rtc_gpio_set_level((gpio_num_t)pin_regulator_enable, 0);
	  delay(60000);
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
      //rtc_gpio_wakeup_enable((gpio_num_t)pin_button, GPIO_INTR_POSEDGE);
      //gpio_isr_handler_add((gpio_num_t)pin_button, this->InterruptHandler(), NULL),
    }
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
};
