
template<int battery_pin>
class Battery {
  public:
    float getBattery() {
      return analogRead(battery_pin) * 2 * 3.3 * 1.1 / 4095;
    }
    uint8_t convert(float batteryValue) {
      if(batteryValue > 4.15) {
        return 10;
      } else if(batteryValue > 4.11) {
        return 9;
      } else if(batteryValue > 4.02) {
        return 8;
      } else if(batteryValue > 3.95) {
        return 7;
      } else if(batteryValue > 3.87) {
        return 6;
      } else if(batteryValue > 3.84) {
        return 5;
      } else if(batteryValue > 3.80) {
        return 4;
      } else if(batteryValue > 3.77) {
        return 3;
      } else if(batteryValue > 3.72) {
        return 2;
      } else if(batteryValue > 3.69) {
        return 1;
      }
      return 0;
    }
};