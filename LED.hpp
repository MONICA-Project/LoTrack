#ifndef _LED_HPP_INCLUDED
#define _LED_HPP_INCLUDED

/// <summary>
/// Class that controls the led or the rgb led.
/// Needs the <typeparamref name="pin_led_red"/> or zero to disable the led complete.
/// <typeparamref name="pin_led_green"/> as a number or zero to disable the green led (fallback to the red one).
/// <typeparamref name="pin_led_blue"/> as a number or zero to disable the blue led (fallback to the red one).
/// </summary>
/// <typeparam name="pin_led_red">Pin number or zero to disable</typeparam>
/// <typeparam name="pin_led_green">Pin number or zero to disable</typeparam>
/// <typeparam name="pin_led_blue">Pin number or zero to disable</typeparam>
template<int pin_led_red, int pin_led_green, int pin_led_blue>
class LedT {
  public:
    static const uint8_t BLACK = 0;
    static const uint8_t BLUE = 1;
    static const uint8_t GREEN = 2;
    static const uint8_t CYAN = 3;
    static const uint8_t RED = 4;
    static const uint8_t MAGENTA = 5;
    static const uint8_t YELLOW = 6;
    static const uint8_t WHITE = 7;

    /// <summary>Constructor for LED class, setup the io pins</summary>
    LedT() {
      this->setupIO();
    }

    /// <summary>Let the led glow 10ms. <typeparamref name="mask"/> is the colormask</summary>
    /// <typeparam name="mask">The colormask in binary, where bit 0 means blue, bit 1 green and bit 2 red.</typeparam>
    void Blink(uint8_t mask) {
      this->Color(mask);
      delay (10);
      this->Color(this->BLACK);
    }

    /// <summary>Let the led glow continiously. <typeparamref name="mask"/> is the colormask</summary>
    /// <typeparam name="mask">The colormask in binary, where bit 0 means blue, bit 1 green and bit 2 red.</typeparam>
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

typedef LedT<pin_ledr, pin_ledg, pin_ledb> Led;

#endif // !_LED_HPP_INCLUDED