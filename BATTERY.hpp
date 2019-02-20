
template<int battery_pin>
class Battery {
  public:
    Battery() {
      if (battery_pin!=0) {
        pinMode(battery_pin, INPUT);
      }
    }
    float GetBattery() {
      if (battery_pin!=0) {
        return analogRead(battery_pin) * 2 * 3.3 * 1.1 / 4095;
      } else {
        return 4.0;
      }
    }
};
