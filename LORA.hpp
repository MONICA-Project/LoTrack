//#include "include/LoRa.h"
#include <LoRa.h>

template<int pin_miso, int pin_mosi, int pin_sck, int pin_ss, int pin_rst, int pin_dio, long band>
class LORA {
public:
  LORA(oledclass* disp) {
    this->display = disp;
    this->lora = new LoRaClass();
    SPI.begin (pin_sck, pin_miso, pin_mosi, pin_ss);
    this->lora->setPins (pin_ss, pin_rst, pin_dio);
  }
  void begin() {
    display->box("Setup Lora!", 80);
    if (!this->lora->begin (band)) {
      display->box("Lora Failed!", 90);
    } else {
      this->lora->setTxPower(17);
      //this->lora->setSignalBandwidth(7.8E3);
      this->lora->enableCrc();
      display->box("Lora successful", 90);
      this->_lora_enabled = true;
    }
  }
  void send(wlanclass* wlan, gpsInfoField gps) {
    this->lora->beginPacket();
    wlan->lock();
    for (int i = 0; i < wlan->size; i++) { //WLAN 12+1+4+1+2+1 = 21 Char
      this->lora->print(wlan->data[i].mac_Address); // 12 Char
      this->lora->print(",");
      this->lora->print(wlan->data[i].mac_RSSI); // 4 Char
      this->lora->print(",");
      this->lora->println(wlan->data[i].mac_Channel); // 2 Char + ln (1 Char)
    }
    wlan->unlock();
    //Gps 18+7+9+1 = 35 Char
    this->lora->print(String(gps.latitude, 10) + "," + String(gps.longitude, 10) + ","); //8+1+8+1 = 18 Char
    this->lora->print(gps.hour < 10 ? String("0") + String(gps.hour) : String(gps.hour)); //2+2+2+1 = 7 Char
    this->lora->print(gps.minute < 10 ? String("0") + String(gps.minute) : String(gps.minute));
    this->lora->print(gps.second < 10 ? String("0") + String(gps.second) + String(",") : String(gps.second) + String(","));
    this->lora->println(String(gps.HDOP, 2) + "," + String(gps.Satellites) + "," + String(gps.gnssFix ? "t" : "f")); //4+1+2+1+1 = 9 Char + LN (1 Char)
    this->lora->endPacket();
  }
private:
  oledclass* display;
  LoRaClass* lora;
  bool _lora_enabled = false;
};
