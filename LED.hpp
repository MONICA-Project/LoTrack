
template<int pin_led>
class LED {
public:
  LED() {
    pinMode(pin_led, OUTPUT);
  }
  void Blink() {
    this->On(); 
    delay (10);
    this->Off();
  }
  void On() {
    digitalWrite(pin_led, HIGH); // turn the LED on (HIGH is the voltage level)
  }
  void Off() {
    digitalWrite(pin_led, LOW); // turn the LED off by making the voltage LOW
  }
};