//#include "include\MicroNMEA.h"
#include <MicroNMEA.h>
#include <mutex>
#include <pthread.h>

template <int serial_pin, int pin_tx, int pin_rx, bool debug>
class GPS {
public:
  GPS(wlanclass* disp) {
    this->display = disp;
    this->hs = new HardwareSerial(serial_pin);
    this->nmea = new MicroNMEA(this->nmeaBuffer, sizeof(this->nmeaBuffer));
  }
  void begin() {
    this->display->box("Gps Setup!", 60);
    this->hs->begin(9600, SERIAL_8N1, pin_rx, pin_tx);
    this->display->box("Gps Successfull", 70);
  }
  void measure() {
    nmea->clear();
    while (true) {
      pthread_mutex_lock(&this->mgp);
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
        this->gpsdata.Satellites = nmea->getNumSatellites();
        this->gpsdata.latitude = ((float)nmea->getLatitude()) / 1000000;
        this->gpsdata.longitude = ((float)nmea->getLongitude()) / 1000000;
        this->gpsdata.hour = nmea->getHour()%24;
        this->gpsdata.minute = nmea->getMinute()%60;
        this->gpsdata.second = nmea->getSecond()%60;
        float hdop_dez = nmea->getHDOP();
        this->gpsdata.HDOP = hdop_dez / 10;
        mtx.unlock();
      }
      pthread_mutex_unlock(&this->mgp);
      delay(1);
    }
  }
  gpsInfoField getGPSData() {
    mtx.lock();
    gpsInfoField g = this->gpsdata;
    mtx.unlock();
    return g;
  }
  pthread_mutex_t mgp;
private:
  HardwareSerial* hs;
  char nmeaBuffer[100];
  MicroNMEA* nmea;
  wlanclass* display;
  gpsInfoField gpsdata;
  std::mutex mtx;
};
