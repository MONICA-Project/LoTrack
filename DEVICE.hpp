
template<int pin_regulator_enable, int pin_button>
class Device {
  public:
    void activateDevice() {
       pinMode(pin_regulator_enable, OUTPUT);
       pinMode(pin_button, INPUT);
       digitalWrite(pin_button, LOW);
       digitalWrite(pin_regulator_enable, HIGH);
    }
    float readButton() {
      return analogRead(pin_button)/ 4095;
    }
};
