#include <ArduinoOTA.h>

template<const char* espname>
class OTA {
public:
  OTA(wlanclass* wlanclass, ledclass * ledclass) {
    this->wlan = wlanclass;
    this->led = ledclass;
  }
  void setup() {
    this->wlan->box("OTA Setup!", 40);
    this->onStart();
    this->onEnd();
    this->onProgress();
    this->onError();
    ArduinoOTA.setHostname(espname);
    ArduinoOTA.begin();
    this->wlan->box("OTA Successfull", 50);
  }
  void onStart() {
    ArduinoOTA.onStart([this]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";
      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      this->wlan->clear();
      this->wlan->drawString(0, 0, "Start updating:" + type);
      this->wlan->display();
    });
  }
  void onEnd() {
    ArduinoOTA.onEnd([this]() {
      this->wlan->drawString(0, 40, "End");
      this->wlan->display();
    });
  }
  void onProgress() {
    ArduinoOTA.onProgress([this](unsigned int progress, unsigned int total) {
      uint8_t p = progress / (total / 100);
      if(p % 2) {
        led->on();
      } else {
        led->off();
      }
      this->wlan->log(String(p) + String(" "));
      this->wlan->box(String("Progess update..."), p);
    });
  }
  void onError() {
    ArduinoOTA.onError([this](ota_error_t error) {
      this->wlan->drawString(0, 40, "Error: ");
      this->wlan->display();
      if (error == OTA_AUTH_ERROR) {
        this->wlan->drawString(60, 40, "Auth Failed");
        this->wlan->display();
      }
      else if (error == OTA_BEGIN_ERROR) {
        this->wlan->drawString(60, 40, "Begin Failed");
        this->wlan->display();
      }
      else if (error == OTA_CONNECT_ERROR) {
        this->wlan->drawString(60, 40, "Connect Failed");
        this->wlan->display();
      }
      else if (error == OTA_RECEIVE_ERROR) {
        this->wlan->drawString(60, 40, "Receive Failed");
        this->wlan->display();
      }
      else if (error == OTA_END_ERROR) {
        this->wlan->drawString(60, 40, "End Failed");
        this->wlan->display();
      }
    });
  }
  void check() {
    ArduinoOTA.handle();
  }
private:
  wlanclass * wlan;
  ledclass * led;
};

// Port defaults to 8266
// ArduinoOTA.setPort(8266);

// Hostname defaults to esp8266-[ChipID]
// ArduinoOTA.setHostname("myesp8266");

// No authentication by default
// ArduinoOTA.setPassword("admin");

// Password can be set with it's md5 value as well
// MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
// ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

// No authentication by default
//ArduinoOTA.setPassword((const char *)"1234");