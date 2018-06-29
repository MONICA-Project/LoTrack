#define DEBUG_OLEDDISPLAY(...) Serial.printf( __VA_ARGS__ )
//#include "include/SSD1306.h"
#include <SSD1306.h>

template <int pin_address, int pin_sda, int pin_scl, int pin_display>
class OLED {
public:
  OLED() {
    this->display_power();
    this->d = new SSD1306Wire(pin_address, pin_sda, pin_scl);
    this->d->init();
    this->d->flipScreenVertically();
    this->d->setFont(ArialMT_Plain_10);

    this->d->clear();
    this->d->setTextAlignment(TEXT_ALIGN_LEFT);
    this->d->setFont(ArialMT_Plain_10);

    this->box("Oled Init!", 10);
  }
  void drawString(int16_t x, int16_t y, String text) {
    this->d->drawString(x, y, text);
  }
  void display() {
    this->d->display();
  }
  void clear() {
    this->d->clear();
  }
  void gps(gpsInfoField gpsInfo) {
    this->d->clear();
    this->d->drawString(0, 0, "GNSS FIX:");
    this->d->drawString(60, 0, String(gpsInfo.gnssFix));
    this->d->drawString(0, 10, "Satellites: ");
    this->d->drawString(60, 10, String(gpsInfo.Satellites));
    this->d->drawString(0, 20, "Lat:");
    this->d->drawString(60, 20, String(gpsInfo.latitude, 6));
    this->d->drawString(0, 30, "Long:");
    this->d->drawString(60, 30, String(gpsInfo.longitude, 6));
    this->d->drawString(0, 40, "HDOP:");
    this->d->drawString(60, 40, String(gpsInfo.HDOP, 2));
    this->d->drawString(0, 50, "Fix Time:");
    this->d->drawString(60, 50, (gpsInfo.hour < 10?"0":"") + String(gpsInfo.hour));
    this->d->drawString(73, 50, ":");
    this->d->drawString(78, 50, (gpsInfo.minute < 10 ? "0":"") + String(gpsInfo.minute));
    this->d->drawString(91, 50, ":");
    this->d->drawString(96, 50, (gpsInfo.second < 10 ? "0":"") + String(gpsInfo.second));
    this->d->display();
    Serial.println("################################################");
    Serial.println("GNSS FIX: " + String(gpsInfo.gnssFix));
    Serial.println("Satellites: " + String(gpsInfo.Satellites));
    Serial.println("Lat: " + String(gpsInfo.latitude, 6));
    Serial.println("Long: " + String(gpsInfo.longitude, 6));
    Serial.println("HDOP: " + String(gpsInfo.HDOP, 6));
    Serial.println("Fix Time: " + String(gpsInfo.hour < 10 ? "0" : "") + String(gpsInfo.hour) + String(gpsInfo.minute < 10 ? "0" : "") + String(gpsInfo.minute) + String(gpsInfo.second < 10 ? "0" : "") + String(gpsInfo.second));
  }
  void box(String text, uint8_t prog) {
    this->d->clear();
    this->drawString(2, 51, text);
    this->d->drawRect(0, 0, 128, 64);
    this->d->drawHorizontalLine(0, 49, 128);
    this->d->drawProgressBar(5, 10, 115, 15, prog);
    this->display();
    Serial.println(text);
  }
private:
  SSD1306Wire* d;
  void display_power() {
    pinMode(pin_display, OUTPUT);
    digitalWrite(pin_display, LOW);
    delay(50);
    digitalWrite(pin_display, HIGH);
  }
};
