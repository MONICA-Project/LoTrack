#include <SSD1306Wire.h>

/// <summary>
/// Class to Write on a OLED Display
/// </summary>
/// <typeparam name="pin_sda">Pin number of SDA pin on the controller</typeparam>
/// <typeparam name="pin_scl">Pin number of SCL pin on the controller</typeparam>
/// <typeparam name="pin_display_reset">Pin number of pin to reset the OLED. If zero display is disabled</typeparam>
template <int pin_sda, int pin_scl, int pin_display_reset>
class OLED {
  public:
    /// <summary>Constructor for OLED class, setup the io pins and init the display</summary>
    OLED() {
      if(pin_display_reset != 0) {
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

    /// <summary>Write a String on the display</summary>
    /// <typeparam name="x">x coordinates from left</typeparam>
    /// <typeparam name="y">y coordinates from top</typeparam>
    /// <typeparam name="text">String to display</typeparam>
    void drawString(int16_t x, int16_t y, String text) {
      if(pin_display != 0) {
        this->d->drawString(x, y, text);
      }
    }

    /// <summary>Call to display the current changes</summary>
    void display() {
      if(pin_display_reset != 0) {
        this->d->display();
      }
    }

    /// <summary>Call to clear the whole display content</summary>
    void clear() {
      if(pin_display_reset != 0) {
        this->d->clear();
      }
    }

    /// <summary>Write a table with gps and battery informations on the screen</summary>
    /// <typeparam name="gpsInfo">struct gpsInfoField, with all nessesary gps informations</typeparam>
    /// <typeparam name="battery">voltage of the battery value</typeparam>
    void gps(gpsInfoField gpsInfo, float battery) {
      if(pin_display_reset != 0) {
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

    /// <summary>Plotting a box with a progessbar and text inside</summary>
    /// <typeparam name="text">Text insode the box</typeparam>
    /// <typeparam name="percent">percent value for the progessbar</typeparam>
    void box(String text, uint8_t percent) {
      if(pin_display_reset != 0) {
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
      if(pin_display_reset != 0) {
        pinMode(pin_display_reset, OUTPUT);
        digitalWrite(pin_display_reset, LOW);
        delay(50);
        digitalWrite(pin_display_reset, HIGH);
      }
    }
};
