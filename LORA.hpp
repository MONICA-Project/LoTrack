#include "include/LoRa.h"

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
  void send(macInfoField* macs, gpsInfoField gps, uint8_t mac_l) {
    this->lora->beginPacket();
    for (int i = 0; i < mac_l; i++) { //WLAN 12+1+4+1+2+1 = 21 Char
      this->lora->print(macs[i].mac_Address); // 12 Char
      this->lora->print(",");
      this->lora->print(macs[i].mac_RSSI); // 4 Char
      this->lora->print(",");
      this->lora->println(macs[i].mac_Channel); // 2 Char + ln (1 Char)
    }
    //Gps 8+1+8+1+6 = 24 Char
    this->lora->print(gps.latitude, 8);
    this->lora->print(",");
    this->lora->print(gps.longitude, 8);
    this->lora->print(",");
    this->lora->print(gps.hour<10?"0"+ String(gps.hour):String(gps.hour));
    this->lora->print(gps.minute<10?"0"+String(gps.minute):String(gps.minute));
    this->lora->println(gps.second<10?"0"+String(gps.second):String(gps.second));
    this->lora->endPacket();
  }
private:
  oledclass* display;
  LoRaClass* lora;
  bool _lora_enabled = false;
};
