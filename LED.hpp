
template<int pin_led>
class LED {
public:
  LED() {
    pinMode(pin_led, OUTPUT);
  }
  void blink() {
    this->on(); 
    delay (10);
    this->off();
  }
  void on() {
    digitalWrite(pin_led, HIGH); // turn the LED on (HIGH is the voltage level)
  }
  void off() {
    digitalWrite(pin_led, LOW); // turn the LED off by making the voltage LOW
  }
};