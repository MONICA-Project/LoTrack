#include <ArduinoOTA.h>

class OTA {
public:
  OTA(wlanclass* disp) {
    this->display = disp;
  }
  void setup() {
    this->display->box("OTA Setup!", 40);
    this->onStart();
    this->onEnd();
    this->onProgress();
    this->onError();
    ArduinoOTA.begin();
    this->display->box("OTA Successfull", 50);
  }
  void onStart() {
    ArduinoOTA.onStart([this]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
      this->display->clear();
      this->display->drawString(0, 0, "Start updating:");
      this->display->display();
    });
  }
  void onEnd() {
    ArduinoOTA.onEnd([this]() {
      Serial.println("\nEnd");
      this->display->drawString(0, 40, "End");
      this->display->display();
    });
  }
  void onProgress() {
    ArduinoOTA.onProgress([this](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
      this->display->clear();
      this->display->drawString(0, 0, "Start updating:");
      this->display->drawString(0, 20, "Progress:");
      this->display->drawString(60, 20, "  ");
      this->display->display();
      this->display->drawString(60, 20, String((progress / (total / 100))));
      this->display->display();
    });
  }
  void onError() {
    ArduinoOTA.onError([this](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      this->display->drawString(0, 40, "Error: ");
      this->display->display();
      if (error == OTA_AUTH_ERROR) {
        Serial.println("Auth Failed");
        this->display->drawString(60, 40, "Auth Failed");
        this->display->display();
      }
      else if (error == OTA_BEGIN_ERROR) {
        Serial.println("Begin Failed");
        this->display->drawString(60, 40, "Begin Failed");
        this->display->display();
      }
      else if (error == OTA_CONNECT_ERROR) {
        Serial.println("Connect Failed");
        this->display->drawString(60, 40, "Connect Failed");
        this->display->display();
      }
      else if (error == OTA_RECEIVE_ERROR) {
        Serial.println("Receive Failed");
        this->display->drawString(60, 40, "Receive Failed");
        this->display->display();
      }
      else if (error == OTA_END_ERROR) {
        Serial.println("End Failed");
        this->display->drawString(60, 40, "End Failed");
        this->display->display();
      }
    });
  }
  void check() {
    ArduinoOTA.handle();
  }
private:
  wlanclass* display;
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