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


template<int pin_miso, int pin_mosi, int pin_sck, int pin_ss, int pin_rst, int pin_dio, long band, const char* espname>
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
      this->lora->setSignalBandwidth(62500);
      this->lora->setSpreadingFactor(8);
      this->lora->setCodingRate4(6);
      this->lora->setTxPower(17);
      this->lora->enableCrc();
      //this->lora->disableCrc();
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
    long startWait = millis();
	while(this->lora->hasChannelActivity()) {
		delay(1);
	}
	long endWait = millis();
	this->lora->idle();
    this->lora->beginPacket();
	this->lora->print(data);
	this->lora->endPacket();
    this->lora->sleep();
	this->wlan->Log(String("################################################\n"));
	this->wlan->Log(String("Waiting: ") + String(endWait-startWait) + String(" ms\n"));
	this->wlan->Log(data + String("\n"));
  }

  void Send(gpsInfoField gps, float batt, bool as_bytes = false) {
    //this->lora->idle();
    //this->lora->beginPacket();
    //if(!as_bytes) {
      String data = String(espname) + "\n" + String(gps.latitude, 6) + "," + String(gps.longitude, 6) + "," + String(gps.time) + "," + String(gps.hdop, 2) + "," + String(gps.height, 1) + "," + String(batt, 2); //Gps 9+9+7+5+4 = 34 Char
      this->Send(data);
	  //this->lora->print(data);
      /// Logging
      //this->wlan->Log(String("################################################\n"));
      //this->wlan->Log(String(espname) + String("\n"));
      //this->wlan->Log(data + String("\n"));
    /*} else {
      uint8_t lora_data[28];
      lora_data[0] = 'b';
      for(uint8_t i = 0; i < 8; i++) {
        if(strlen(espname) > i) {
          lora_data[i + 1] = esp_name[i];
        } else {
          lora_data[i + 1] = 0;
        }
      }
      uint64_t lat = *(uint64_t*)&gps.latitude;  lora_data[13] = (lat >> 0) & 0x7f; lora_data[12] = (lat >> 7) & 0x7f; lora_data[11] = (lat >> 14) & 0x7f; lora_data[10] = (lat >> 21) & 0x7f; lora_data[9] = (lat >> 28) & 0x0f; //5 Bytes Lat
      uint64_t lon = *(uint64_t*)&gps.longitude; lora_data[18] = (lon >> 0) & 0x7f; lora_data[17] = (lon >> 7) & 0x7f; lora_data[16] = (lon >> 14) & 0x7f; lora_data[15] = (lon >> 21) & 0x7f; lora_data[14] = (lon >> 28) & 0x0f; //5 Bytes Lon
      lora_data[19] = String(gps.time.substring(0, 2)).toInt();
      lora_data[20] = String(gps.time.substring(2, 4)).toInt();
      lora_data[21] = String(gps.time.substring(4, 6)).toInt();
      uint64_t hdo = *(uint64_t*)&gps.hdop;      lora_data[26] = (hdo >> 0) & 0x7f; lora_data[25] = (hdo >> 7) & 0x7f; lora_data[24] = (hdo >> 14) & 0x7f; lora_data[23] = (hdo >> 21) & 0x7f; lora_data[22] = (hdo >> 28) & 0x0f; //5 Bytes Hdop
      lora_data[27] = batt;
      this->lora->write(lora_data, 28);
      /// Logging
      this->wlan->Log(String("################################################\n"));
      String g;
      for(uint8_t i = 0; i < 28; i++) {
        g = g + String(lora_data[i], HEX) + String(" ");
      }
      this->wlan->Log(g + String("\n"));
    }*/
    //this->lora->endPacket();
    //this->lora->sleep();
  }

  void Send(uint8_t version, String ip, String ssid, bool wififlag, float battery, int32_t freqoffset) {
    this->lora->idle();
    this->lora->beginPacket();
    this->lora->println("deb");
    this->lora->println(espname);
    this->lora->print(String(version)+","+ip+","+ssid+","+(wififlag?"t":"f")+","+String(battery,2)+","+String(freqoffset));
    this->lora->endPacket();
    this->lora->sleep();
  }
  #pragma endregion

private:
  wlanclass * wlan;
  LoRaClass * lora;
  Storage * storage;
  bool _lora_enabled = false;
};
