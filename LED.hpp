
template<int pin_led>
class LED {
public:
  LED() {
    pinMode(pin_led, OUTPUT);
  }
  void blink() {
    digitalWrite(pin_led, HIGH); // turn the LED on (HIGH is the voltage level)
    delay (10); // wait for a second
    digitalWrite(pin_led, LOW); // turn the LED off by making the voltage LOW
  }
};