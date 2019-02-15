
template<int pin_led, int pin_led_rd, int pin_led_gr, int pin_led_bl>
class LED {
  public:
    LED() {
      pinMode(pin_led, OUTPUT);
      pinMode(pin_led_rd, OUTPUT);
      pinMode(pin_led_gr, OUTPUT);
      pinMode(pin_led_bl, OUTPUT);    
    }
    void Blink(uint8_t c) {
      this->On(c);
      delay (10);
      this->Off(c);
    }
    void On(uint8_t c) {
      
      if (c == 'r') {
        digitalWrite(pin_led_rd, HIGH); // turn the LED on (HIGH is the voltage level)
        digitalWrite(pin_led_gr, LOW); // turn the LED on (HIGH is the voltage level)
        digitalWrite(pin_led_bl, LOW); // turn the LED on (HIGH is the voltage level)
      }
      else if (c =='b'){
        digitalWrite(pin_led_rd, LOW); // turn the LED on (HIGH is the voltage level)
        digitalWrite(pin_led_gr, LOW); // turn the LED on (HIGH is the voltage level)
        digitalWrite(pin_led_bl, LOW); // turn the LED on (HIGH is the voltage level)
      }
      else if (c =='g'){
        digitalWrite(pin_led_rd, LOW); // turn the LED on (HIGH is the voltage level)
        digitalWrite(pin_led_gr, HIGH); // turn the LED on (HIGH is the voltage level)
        digitalWrite(pin_led_bl, LOW); // turn the LED on (HIGH is the voltage level)
      }
      else{
        digitalWrite(pin_led, HIGH); // turn the LED on (HIGH is the voltage level)
      }

    }
    void Off(char c) {
      

      if (c == 'r') {
        digitalWrite(pin_led_rd, LOW); // turn the LED on (HIGH is the voltage level)
      }
      else if (c =='g'){
        digitalWrite(pin_led_gr, LOW); // turn the LED on (HIGH is the voltage level)
      }
      else if (c =='b'){
        digitalWrite(pin_led_bl, LOW); // turn the LED on (HIGH is the voltage level)
      }
      else{
        digitalWrite(pin_led, LOW); // turn the LED off by making the voltage LOW
      }
      
    }

    void Red() {
      digitalWrite(pin_led_rd, HIGH); // turn the LED on (HIGH is the voltage level)
      digitalWrite(pin_led_gr, LOW); // turn the LED on (HIGH is the voltage level)
      digitalWrite(pin_led_bl, LOW); // turn the LED on (HIGH is the voltage level)

    }

    void Blue() {
      digitalWrite(pin_led_rd, LOW); // turn the LED on (HIGH is the voltage level)
      digitalWrite(pin_led_gr, LOW); // turn the LED on (HIGH is the voltage level)
      digitalWrite(pin_led_bl, HIGH); // turn the LED on (HIGH is the voltage level)

    }

    void Green() {
      digitalWrite(pin_led_rd, LOW); // turn the LED on (HIGH is the voltage level)
      digitalWrite(pin_led_gr, HIGH); // turn the LED on (HIGH is the voltage level)
      digitalWrite(pin_led_bl, LOW); // turn the LED on (HIGH is the voltage level)

    }
};
