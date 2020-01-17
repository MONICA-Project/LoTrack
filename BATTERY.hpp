#ifndef _BATTERY_HPP_INCLUDED
#define _BATTERY_HPP_INCLUDED

#include "STORAGE.hpp"

/// <summary>
/// Class that read out the battery voltage, needs the <typeparamref name="battery_pin"/> as a number.
/// If zero a fix value of 4V is readout.
/// </summary>
/// <typeparam name="battery_pin">Pin number or zero to disable</typeparam>
template<int battery_pin>
class BatteryT {
  public:
    /// <summary>Constructor of the battery voltage class, setups the Pin</summary>
    BatteryT(Storage * storage) {
      this->storage = storage;
      this->SetupIO();
    }

    /// <summary>Begin Battery, Readout the offset</summary>
    void Begin() {
      this->batteryoffset = this->storage->GetBattoffset();
    }

    /// <summary>Get the voltage of the battery</summary>
    /// <returns>Returns voltage or zero.</returns>
    float_t GetBattery() {
      if (battery_pin != 0) {
        return (analogRead(battery_pin) * 2 * 3.3 * 1.1 / 4095) + batteryoffset;
      } else {
        return 4.0;
      }
    }

    /// <summary>Set the internal offset</summary>
    /// <typeparam name="o">offset in volt</typeparam>
    void SetOffset(float_t o) {
      this->batteryoffset = o;
    }
  private:
    float_t batteryoffset = 0;
    Storage * storage;

    void SetupIO() {
      if(battery_pin != 0) {
        pinMode(battery_pin, INPUT);
      }
    }
};

typedef BatteryT<pin_batt> Battery;

#endif // !_BATTERY_HPP_INCLUDED
