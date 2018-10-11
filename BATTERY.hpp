
template<int battery_pin, bool has_battery>
class Battery {
  public:
    float GetBattery() {
      if(has_battery) {
        return analogRead(battery_pin) * 2 * 3.3 * 1.1 / 4095;
      } else {
        return 4.0;
      }
    }
};