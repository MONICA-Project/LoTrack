#include <Preferences.h>

/// <summary>Class to Store values on Non Volatile Memory</summary>
class Storage {
  public:
    /// <summary>Constructor of Storage class</summary>
    Storage() {
      this->preferences = new Preferences();
    }

    /// <summary>Start Storage, init variables if there not set yet</summary>
    void Begin() {
      this->preferences->begin("finetune", false);
      if(this->preferences->getLong("finetune", -2147483648) == -2147483648) {
        this->preferences->putLong("finetune", 0);
      }
      if (this->preferences->getFloat("batteryoffset", -1234) == -1234) {
        this->preferences->putFloat("batteryoffset", 0);
      }
    }

    /// <summary>Get the frequency offset value</summary>
    /// <returns>return the offset in hz</returns>
    int32_t ReadOffsetFreq() {
      return this->preferences->getLong("finetune", 0);
    }

    /// <summary>Get the battery offset value</summary>
    /// <returns>return the offset in volt</returns>
    float_t ReadBatteryOffset() {
      return this->preferences->getFloat("batteryoffset", 0);
    }

    /// <summary>Set the frequency offset</summary>
    /// <typeparam name="o">Value in hz as offset</typeparam>
    void WriteOffsetFreq(int32_t o) {
      this->preferences->putLong("finetune", o);
    }

    /// <summary>Set the battery offset</summary>
    /// <typeparam name="o">value in volt as offset</typeparam>
    void WriteBatteryOffset(float_t o) {
      this->preferences->putFloat("batteryoffset", o);
    }
  private:
    Preferences * preferences = NULL;
};