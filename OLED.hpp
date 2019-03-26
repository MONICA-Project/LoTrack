#include <SSD1306Wire.h>
  
template <int pin_sda, int pin_scl, int pin_display>
class OLED {
  public:
    OLED() {
      if(pin_display != 0) {
        this->display_power();
        this->d = new SSD1306Wire(0x3c, pin_sda, pin_scl);
        this->d->init();
        this->d->setBrightness(1);
        this->d->setFont(ArialMT_Plain_10);

        this->d->clear();
        this->d->setTextAlignment(TEXT_ALIGN_LEFT);
        this->d->setFont(ArialMT_Plain_10);

        this->box("Oled Init!", 10);
      }
    }

    void drawString(int16_t x, int16_t y, String text) {
      if(pin_display != 0) {
        this->d->drawString(x, y, text);
      }
    }

    void display() {
      if(pin_display != 0) {
        this->d->display();
      }
    }

    void clear() {
      if(pin_display != 0) {
        this->d->clear();
      }
    }

    void gps(gpsInfoField gpsInfo, float battery) {
      if(pin_display != 0) {
        this->d->clear();
        this->d->drawString(0, 0, "GNSS FIX:");
        this->d->drawString(60, 0, String(gpsInfo.fix ? "true" : "false"));
        this->d->drawString(0, 8, "Satellites: ");
        this->d->drawString(60, 8, String(gpsInfo.Satellites));
        this->d->drawString(0, 16, "Lat:");
        this->d->drawString(60, 16, String(gpsInfo.latitude, 6));
        this->d->drawString(0, 24, "Long:");
        this->d->drawString(60, 24, String(gpsInfo.longitude, 6));
        this->d->drawString(0, 32, "HDOP:");
        this->d->drawString(60, 32, String(gpsInfo.hdop, 2));
        this->d->drawString(0, 40, "Fix Time:");
        this->d->drawString(60, 40, gpsInfo.time);
        this->d->drawString(0, 48, String("Battery:"));
        this->d->drawString(60, 48, String(battery, 2));
        this->d->display();
      }
    }

    void box(String text, uint8_t percent) {
      if(pin_display != 0) {
        this->d->clear();
        this->drawString(2, 51, text);
        this->d->drawRect(0, 0, 128, 64);
        this->d->drawHorizontalLine(0, 49, 128);
        this->d->drawProgressBar(5, 10, 115, 15, percent);
        this->display();
      }
    }
  private:
    SSD1306Wire* d = NULL;
    void display_power() {
      if(pin_display != 0) {
        pinMode(pin_display, OUTPUT);
        digitalWrite(pin_display, LOW);
        delay(50);
        digitalWrite(pin_display, HIGH);
      }
    }
};
