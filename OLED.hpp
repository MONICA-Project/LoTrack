#ifndef DISPLAY_OFF
  #define DEBUG_OLEDDISPLAY(...) Serial.printf( __VA_ARGS__ )
  //#include "include/SSD1306.h"
  #include <SSD1306.h>
#endif // !DISPLAY_OFF

template <int pin_address, int pin_sda, int pin_scl, int pin_display, bool silent>
class OLED {
public:
  OLED() {
    #ifndef DISPLAY_OFF
      this->display_power();
      this->d = new SSD1306Wire(pin_address, pin_sda, pin_scl);
      this->d->init();
      this->d->setBrightness(1);
      this->d->setFont(ArialMT_Plain_10);

      this->d->clear();
      this->d->setTextAlignment(TEXT_ALIGN_LEFT);
      this->d->setFont(ArialMT_Plain_10);

      this->box("Oled Init!", 10);
    #endif // !DISPLAY_OFF
  }
  void drawString(int16_t x, int16_t y, String text) {
    #ifndef DISPLAY_OFF
      this->d->drawString(x, y, text);
    #endif // !DISPLAY_OFF
  }
  void display() {
    #ifndef DISPLAY_OFF
      this->d->display();
    #endif // !DISPLAY_OFF
  }
  void clear() {
    #ifndef DISPLAY_OFF
      this->d->clear();
    #endif // !DISPLAY_OFF
  }
  void gps(gpsInfoField gpsInfo, uint8_t networks) {
    #ifndef DISPLAY_OFF
      this->d->clear();
      this->d->drawString(0, 0, "GNSS FIX:");
      this->d->drawString(60, 0, String(gpsInfo.gnssFix ? "true" : "false"));
      this->d->drawString(0, 8, "Satellites: ");
      this->d->drawString(60, 8, String(gpsInfo.Satellites));
      this->d->drawString(0, 16, "Lat:");
      this->d->drawString(60, 16, String(gpsInfo.latitude, 6));
      this->d->drawString(0, 24, "Long:");
      this->d->drawString(60, 24, String(gpsInfo.longitude, 6));
      this->d->drawString(0, 32, "HDOP:");
      this->d->drawString(60, 32, String(gpsInfo.HDOP, 2));
      this->d->drawString(0, 40, "Fix Time:");
      String time = String((gpsInfo.hour < 10 ? "0" : "") + String(gpsInfo.hour)) + ":" + String((gpsInfo.minute < 10 ? "0" : "") + String(gpsInfo.minute)) + ":" + String((gpsInfo.second < 10 ? "0" : "") + String(gpsInfo.second));
      this->d->drawString(60, 40, time);
      this->d->drawString(0, 48, String("Networks:"));
      this->d->drawString(60, 48, String(networks));
      this->d->display();
      if (!silent) {
        Serial.println("################################################");
        Serial.println("GNSS FIX: " + String(gpsInfo.gnssFix));
        Serial.println("Satellites: " + String(gpsInfo.Satellites));
        Serial.println("Lat: " + String(gpsInfo.latitude, 6));
        Serial.println("Long: " + String(gpsInfo.longitude, 6));
        Serial.println("HDOP: " + String(gpsInfo.HDOP, 6));
        Serial.println("Fix Time: " + String(gpsInfo.hour < 10 ? "0" : "") + String(gpsInfo.hour) + String(gpsInfo.minute < 10 ? "0" : "") + String(gpsInfo.minute) + String(gpsInfo.second < 10 ? "0" : "") + String(gpsInfo.second));
      }
    #endif // !DISPLAY_OFF
  }
  void box(String text, uint8_t prog) {
    #ifndef DISPLAY_OFF
      this->d->clear();
      this->drawString(2, 51, text);
      this->d->drawRect(0, 0, 128, 64);
      this->d->drawHorizontalLine(0, 49, 128);
      this->d->drawProgressBar(5, 10, 115, 15, prog);
      this->display();
    #endif // !DISPLAY_OFF
    if (!silent) {
      Serial.println(text);
    }
  }
private:
  #ifndef DISPLAY_OFF
    SSD1306Wire* d;
    void display_power() {
      pinMode(pin_display, OUTPUT);
      digitalWrite(pin_display, LOW);
      delay(50);
      digitalWrite(pin_display, HIGH);
    }
  #endif // !DISPLAY_OFF
};
