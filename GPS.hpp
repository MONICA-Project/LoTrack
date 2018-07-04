//#include "include\MicroNMEA.h"
#include <MicroNMEA.h>
#include <mutex>

template <int serial_pin, int pin_tx, int pin_rx, bool debug>
class GPS {
public:
  GPS(oledclass* disp) {
    this->display = disp;
    this->hs = new HardwareSerial(serial_pin);
    this->nmea = new MicroNMEA(this->nmeaBuffer, sizeof(this->nmeaBuffer));
  }
  void begin() {
    display->box("Gps Setup!", 60);
    this->hs->begin(9600, SERIAL_8N1, pin_rx, pin_tx);
    display->box("Gps Successfull", 70);
  }
  void measure() {
    nmea->clear();
    while (true) {
      char c = this->hs->read();
      if (debug) {
        if (c != 255) {
          Serial.print(c);
        }
        while(Serial.available()) {
          this->hs->write(Serial.read());
        }
      }
      if (nmea->process(c)) {
        mtx.lock();
        this->gpsdata.gnssFix = nmea->isValid();
        if (this->gpsdata.gnssFix) {
          this->gpsdata.Satellites = nmea->getNumSatellites();
          float latitude_mdeg = nmea->getLatitude();
          float longitude_mdeg = nmea->getLongitude();
          this->gpsdata.latitude = (latitude_mdeg / 1000000);
          this->gpsdata.longitude = (longitude_mdeg / 1000000);
          this->gpsdata.hour = nmea->getHour()%60;
          this->gpsdata.minute = nmea->getMinute()%60;
          this->gpsdata.second = nmea->getSecond()%60;
          float hdop_dez = nmea->getHDOP();
          this->gpsdata.HDOP = hdop_dez / 10;
        }
        mtx.unlock();
      }
      delay(1);
    }
  }
  gpsInfoField getGPSData() {
    mtx.lock();
    gpsInfoField g = this->gpsdata;
    mtx.unlock();
    return g;
  }
private:
  HardwareSerial* hs;
  char nmeaBuffer[100];
  MicroNMEA* nmea;
  oledclass* display;
  gpsInfoField gpsdata;
  std::mutex mtx;
};
