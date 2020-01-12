#ifndef _STORAGE_HPP_INCLUDED
#define _STORAGE_HPP_INCLUDED

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
      this->freqoffset = this->preferences->getLong("finetune", -2147483648);
      this->battoffset = this->preferences->getFloat("batteryoffset", -1234);
      this->espname = this->preferences->getString("espname", String(""));
      if(this->freqoffset == -2147483648) {
        this->SetFreqoffset(0);
      }
      if (this->battoffset == -1234) {
        this->SetBattoffset(0);
      }
      if (this->espname.equals("")) {
        this->SetEspname(String("xx"));
      }
    }

    /// <summary>Get the frequency offset value</summary>
    /// <returns>return the offset in hz</returns>
    int32_t GetFreqoffset() {
      return this->freqoffset;
    }

    /// <summary>Get the battery offset value</summary>
    /// <returns>return the offset in volt</returns>
    float_t GetBattoffset() {
      return this->battoffset;
    }

    /// <summary>Get the espname</summary>
    /// <returns>return the name as string</returns>
    String GetEspname() {
      return this->espname;
    }

    /// <summary>Set the frequency offset</summary>
    /// <typeparam name="o">Value in hz as offset</typeparam>
    void SetFreqoffset(int32_t o) {
      this->preferences->putLong("finetune", o);
      this->freqoffset = o;
    }

    /// <summary>Set the battery offset</summary>
    /// <typeparam name="o">value in volt as offset</typeparam>
    void SetBattoffset(float_t o) {
      this->preferences->putFloat("batteryoffset", o);
      this->battoffset = o;
    }

    /// <summary>Set the espname</summary>
    /// <typeparam name="o">value name as string</typeparam>
    void SetEspname(String o) {
      this->preferences->putString("espname", o);
      this->espname = o;
    }
  private:
    Preferences * preferences = NULL;
    int32_t freqoffset = 0;
    float_t battoffset = 0.0;
    String espname;
};

#endif // !_STORAGE_HPP_INCLUDED