/// <summary>
/// Class that read out the battery voltage, needs the <typeparamref name="battery_pin"/> as a number.
/// If zero a fix value of 4V is readout.
/// </summary>
/// <typeparam name="battery_pin">Pin number or zero to disable</typeparam>
template<int battery_pin>
class Battery {
  public:
    /// <summary>Constructor of the battery voltage class, setups the Pin</summary>
    Battery() {
      this->SetupIO();
    }

    /// <summary>Get the voltage of the battery</summary>
    /// <returns>Returns voltage or zero.</returns>
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
