
template<int pin_led_red, int pin_led_green, int pin_led_blue>
class LED {
  public:
    static const uint8_t BLACK = 0;
    static const uint8_t BLUE = 1;
    static const uint8_t GREEN = 2;
    static const uint8_t CYAN = 3;
    static const uint8_t RED = 4;
    static const uint8_t MAGENTA = 5;
    static const uint8_t YELLOW = 6;
    static const uint8_t WHITE = 7;

    LED() {
      this->setupIO();
    }

    void Blink(uint8_t mask) {
      this->Color(mask);
      delay (10);
      this->Color(this->BLACK);
    }

    void Color(uint8_t mask) {
      if(pin_led_green == 0 && mask & (1 << 1)) {
        mask |= this->RED;
      }
      if(pin_led_blue == 0 && mask & (1 << 0)) {
        mask |= this->RED;
      }
      (mask & (1 << 0)) ? this->BlueOn() : this->BlueOff();
      (mask & (1 << 1)) ? this->GreenOn() : this->GreenOff();
      (mask & (1 << 2)) ? this->RedOn() : this->RedOff();
    }

  private:
    void setupIO() {
      if(pin_led_red != 0) {
        pinMode(pin_led_red, OUTPUT);
      }
      if(pin_led_green != 0) {
        pinMode(pin_led_green, OUTPUT);
      }
      if(pin_led_blue != 0) {
        pinMode(pin_led_blue, OUTPUT);
      }
    }

    #pragma region LED-Drives
    void RedOn() {
      if(pin_led_red != 0) {
        digitalWrite(pin_led_red, HIGH);
      }
    }

    void RedOff() {
      if(pin_led_red != 0) {
        digitalWrite(pin_led_red, LOW);
      }
    }

    void GreenOn() {
      if(pin_led_green != 0) {
        digitalWrite(pin_led_green, HIGH);
      }
    }

    void GreenOff() {
      if(pin_led_green != 0) {
        digitalWrite(pin_led_green, LOW);
      }
    }

    void BlueOn() {
      if(pin_led_blue != 0) {
        digitalWrite(pin_led_blue, HIGH);
      }
    }

    void BlueOff() {
      if(pin_led_blue != 0) {
        digitalWrite(pin_led_blue, LOW);
      }
    }
    #pragma endregion
};
