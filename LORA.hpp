//#include "include/LoRa.h"
#include <LoRa.h>

// https://www.semtech.com/products/wireless-rf/lora-transceivers/SX1272
// https://www.semtech.com/uploads/documents/sx1272.pdf
// The SX1272 offers three bandwidth options of 125 kHz, 250 kHz, and 500 kHz with spreading factors ranging from 6 to 12.
// The SX1273 offers the same bandwidth options with spreading factors from 6 to 9.

// https://www.semtech.com/products/wireless-rf/lora-transceivers/SX1276
// https://www.semtech.com/uploads/documents/DS_SX1276-7-8-9_W_APP_V5.pdf
// The SX1276 and SX1279 offer bandwidth options ranging from 7.8 kHz to 500 kHz with spreading factors ranging from 6 to 12, and covering all available frequency bands.
// The SX1277 offers the same bandwidth and frequency band options with spreading factors from 6 to 9. 
// The SX1278 offers bandwidths and spreading factor options, but only covers the lower UHF bands.


template<int pin_miso, int pin_mosi, int pin_sck, int pin_ss, int pin_rst, int pin_dio, long band, const char* espname, bool lbt, bool binary>
class LORA {
public:
  LORA(wlanclass* wlanclass, Storage * storage) {
    this->wlan = wlanclass;
    this->storage = storage;
    this->lora = new LoRaClass();
    SPI.begin (pin_sck, pin_miso, pin_mosi, pin_ss);
    this->lora->setPins (pin_ss, pin_rst, pin_dio);
  }

  void Begin() {
    this->wlan->Box("Setup Lora!", 80);
    if (!this->lora->begin (band + this->storage->ReadOffsetFreq())) {
      this->wlan->Box("Lora Failed!", 90);
    } else {
      this->lora->setSignalBandwidth(125000);
      this->lora->setSpreadingFactor(10);
      this->lora->setCodingRate4(7);
      this->lora->setTxPower(20);
      this->lora->enableCrc();
      this->wlan->Box("Lora successful", 90);
      this->_lora_enabled = true;
    }
  }

  #pragma region Debug-Mode for Frequenztuning
  void Debugmode() {
    this->lora->setSignalBandwidth(1);
  }

  void SetFreqOffset(int32_t o) {
    this->lora->setFrequency(band + o);
  }

  void DebugSend() {
    this->lora->idle();
    this->lora->beginPacket();
    this->lora->print("TEST TEST TEST");
    this->lora->endPacket();
    this->lora->sleep();
  }
  #pragma endregion

  #pragma region Send Data
  void Send(String data) {
    long startWait, endWait;
    if(lbt) {
        startWait = millis();
        while(this->lora->hasChannelActivity()) {
          delay(1);
        }
        endWait = millis();
    }
    this->lora->idle();
    this->lora->beginPacket();
    this->lora->print(data);
    this->lora->endPacket();
    this->lora->sleep();
    this->wlan->Log(String("################################################\n"));
    if(lbt) {
        this->wlan->Log(String("Waiting: ") + String(endWait-startWait) + String(" ms\n"));
    }
    this->wlan->Log(data + String("\n"));
  }

  void Send(uint8_t* data, uint8_t size) {
    long startWait, endWait;
    if(lbt) {
      startWait = millis();
      while(this->lora->hasChannelActivity()) {
        delay(1);
      }
      endWait = millis();
    }
    this->lora->idle();
    this->lora->beginPacket();
    this->lora->write(data, size);
    this->lora->endPacket();
    this->lora->sleep();
    this->wlan->Log(String("################################################\n"));
    if(lbt) {
      this->wlan->Log(String("Waiting: ") + String(endWait - startWait) + String(" ms\n"));
    }

    String g;
    for(uint8_t i = 0; i < size; i++) {
      g = g + String(data[i], HEX) + String(" ");
    }
    this->wlan->Log(g + String("\n"));
  }


  void Send(gpsInfoField gps, float batt) {
    if(binary) {
      //Data 1+2+4+4+1+2+3+3+1 = 21 Char
      uint8_t lora_data[21];
      lora_data[0] = 'b';
      for(uint8_t i = 0; i < 2; i++) {
        if(strlen(espname) > i) {
          lora_data[i + 1] = esp_name[i];
        } else {
          lora_data[i + 1] = 0;
        }
      }
      uint64_t lat = *(uint64_t*)&gps.latitude;  lora_data[3]  = (lat >> 0) & 0xFF; lora_data[4]  = (lat >> 8) & 0xFF; lora_data[5]  = (lat >> 16) & 0xFF; lora_data[6]  = (lat >> 24) & 0xFF;
      uint64_t lon = *(uint64_t*)&gps.longitude; lora_data[7] = (lon >> 0) & 0xFF; lora_data[8]  = (lon >> 8) & 0xFF; lora_data[9]  = (lon >> 16) & 0xFF; lora_data[10]  = (lon >> 24) & 0xFF; 
      if(gps.hdop >= 25.5) { lora_data[11] = 255; } else if(gps.hdop <= 25.5 && gps.hdop > 0){ lora_data[11] = (uint8_t)(gps.hdop * 10); } else { lora_data[11] = 0; }
      lora_data[12] = (uint8_t)((((uint16_t)(gps.height * 10)) >> 0) & 0xFF); lora_data[13] = (uint8_t)((((uint16_t)(gps.height * 10)) >> 8) & 0xFF);
      lora_data[14] = String(gps.time.substring(0, 2)).toInt(); lora_data[15] = String(gps.time.substring(2, 4)).toInt(); lora_data[16] = String(gps.time.substring(4, 6)).toInt();
      lora_data[17] = gps.day; lora_data[18] = gps.month; lora_data[19] = (uint8_t)(gps.year - 2000);
      lora_data[20] = (uint8_t)((batt * 100)-230);
      this->Send(lora_data, 21);
    } else {
      //Gps 9+9+7+5+4 = 34 Char
      this->Send(String(espname) + "\n" + String(gps.latitude, 6) + "," + String(gps.longitude, 6) + "," + String(gps.time) + "," + String(gps.hdop, 2) + "," + String(gps.height, 1) + "," + String(batt, 2));
    }
  }

  void Send(uint8_t version, String ip, String ssid, bool wififlag, float battery, int32_t freqoffset) {
    this->Send("deb\n" + String(espname) + "\n" + String(version) + "," + ip + "," + ssid + "," + (wififlag ? "t" : "f") + "," + String(battery, 2) + "," + String(freqoffset));
  }
  #pragma endregion

private:
  wlanclass * wlan;
  LoRaClass * lora;
  Storage * storage;
  bool _lora_enabled = false;
};
