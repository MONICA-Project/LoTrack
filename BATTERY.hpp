
template<int battery_pin>
class Battery {
  public:
    Battery() {
      this->SetupIO();
    }
    float GetBattery() {
      if (battery_pin != 0) {
        return analogRead(battery_pin) * 2 * 3.3 * 1.1 / 4095;
      } else {
        return 4.0;
      }
    }
  private:
    void SetupIO() {
      if(battery_pin != 0) {
        pinMode(battery_pin, INPUT);
      }
    }
};
