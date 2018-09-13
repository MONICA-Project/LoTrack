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
  LORA(wlanclass* wlanclass) {
    this->wlan = wlanclass;
    this->lora = new LoRaClass();
    SPI.begin (pin_sck, pin_miso, pin_mosi, pin_ss);
    this->lora->setPins (pin_ss, pin_rst, pin_dio);
  }
  void begin() {
    this->wlan->box("Setup Lora!", 80);
    if (!this->lora->begin (band)) {
      this->wlan->box("Lora Failed!", 90);
    } else {
      this->lora->setSignalBandwidth(62500);
      this->lora->setSpreadingFactor(8);
      this->lora->setCodingRate4(6);
      this->lora->setTxPower(17);
      this->lora->enableCrc();
      //this->lora->disableCrc();
      this->wlan->box("Lora successful", 90);
      this->_lora_enabled = true;
    }
  }
  void send(gpsInfoField gps, float batt, bool as_bytes = false) {
    this->lora->idle();
    this->lora->beginPacket();
    if (!as_bytes) {
      this->lora->println(espname);
      //Gps 18+7+9+1 = 35 Char
      this->lora->print(String(gps.latitude, 10) + "," + String(gps.longitude, 10) + ","); //8+1+8+1 = 18 Char
      if (gps.hour < 10) { //2+2+2+1 = 7 Char
        this->lora->print("0");
      }
      this->lora->print(gps.hour);
      if (gps.minute < 10) {
        this->lora->print("0");
      }
      this->lora->print(gps.minute);
      if (gps.second < 10) {
        this->lora->print("0");
      }
      this->lora->print(gps.second);
      this->lora->print(",");
      this->lora->print(String(gps.HDOP, 2) + "," + String(batt,2)); //4+1+2+1+1 = 9 Char + LN (1 Char)
      /// Logging
      this->wlan->log(String("################################################\n"));
      this->wlan->log(String(espname) + String("\n"));
      String g = String(gps.latitude, 10) + String(",") + String(gps.longitude, 10) + String(",");
      if (gps.hour < 10) {
        g += String("0");
      }
      g += String(gps.hour);
      if (gps.minute < 10) {
        g += String("0");
      }
      g += String(gps.minute);
      if (gps.second < 10) {
        g += String("0");
      }
      g += String(gps.second) + String(",") + String(gps.HDOP, 2) + String(",") + String(batt,2);
      this->wlan->log(g + String("\n"));
    }
    else {
      uint8_t lora_data[28];
      lora_data[0] = 'b';
      for (uint8_t i = 0; i < 8; i++) {
        if (strlen(espname) > i) {
          lora_data[i+1] = esp_name[i];
        }
        else {
          lora_data[i+1] = 0;
        }
      }
      uint64_t lat = *(uint64_t*)&gps.latitude;  lora_data[13] = (lat >> 0) & 0x7f; lora_data[12] = (lat >> 7) & 0x7f; lora_data[11] = (lat >> 14) & 0x7f; lora_data[10] = (lat >> 21) & 0x7f; lora_data[9] =  (lat >> 28) & 0x0f; //5 Bytes Lat
      uint64_t lon = *(uint64_t*)&gps.longitude; lora_data[18] = (lon >> 0) & 0x7f; lora_data[17] = (lon >> 7) & 0x7f; lora_data[16] = (lon >> 14) & 0x7f; lora_data[15] = (lon >> 21) & 0x7f; lora_data[14] = (lon >> 28) & 0x0f; //5 Bytes Lon
      lora_data[19] = gps.hour;
      lora_data[20] = gps.minute;
      lora_data[21] = gps.second;
      uint64_t hdo = *(uint64_t*)&gps.HDOP;      lora_data[26] = (hdo >> 0) & 0x7f; lora_data[25] = (hdo >> 7) & 0x7f; lora_data[24] = (hdo >> 14) & 0x7f; lora_data[23] = (hdo >> 21) & 0x7f; lora_data[22] = (hdo >> 28) & 0x0f; //5 Bytes Hdop
      lora_data[27] = batt;
      this->lora->write(lora_data, 28);
      /// Logging
      this->wlan->log(String("################################################\n"));
      String g;
      for(uint8_t i = 0; i < 28; i++) {
        g = g + String(lora_data[i], HEX) + String(" ");
      }
      this->wlan->log(g + String("\n"));
    }
    this->lora->endPacket();
    this->lora->sleep();
  }
private:
  wlanclass * wlan;
  LoRaClass * lora;
  bool _lora_enabled = false;
};
