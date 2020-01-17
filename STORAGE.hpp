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
      if (this->preferences->getBytes("cryptokey", this->cryptokey, 32) == 0) {
        uint8_t key[32] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF,0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF };
        this->SetKey(key);
      }
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

    /// <summary>Get the cryptokey</summary>
    /// <returns>return a uint8_t[32] with the key</returns>
    uint8_t* GetKey() {
      return this->cryptokey;
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

    /// <summary>Set the espname</summary>
    /// <typeparam name="o">value name as string</typeparam>
    void SetKey(uint8_t* k) {
      this->preferences->putBytes("cryptokey", k, 32);
      this->cryptokey = k;
    }
  private:
    Preferences * preferences = NULL;
    int32_t freqoffset = 0;
    float_t battoffset = 0.0;
    String espname;
    uint8_t* cryptokey = new uint8_t[32];
};

#endif // !_STORAGE_HPP_INCLUDED