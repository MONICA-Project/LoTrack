#include <ArduinoOTA.h>

class OTA {
public:
  OTA(oledclass* disp) {
    this->display = disp;
  }
  void setup() {
    display->box("OTA Setup!", 40);
    this->onStart();
    this->onEnd();
    this->onProgress();
    this->onError();
    ArduinoOTA.begin();
    display->box("OTA Successfull", 50);
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
      display->clear();
      display->drawString(0, 0, "Start updating:");
      display->display();
    });
  }
  void onEnd() {
    ArduinoOTA.onEnd([this]() {
      Serial.println("\nEnd");
      display->drawString(0, 40, "End");
      display->display();
    });
  }
  void onProgress() {
    ArduinoOTA.onProgress([this](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
      display->clear();
      display->drawString(0, 0, "Start updating:");
      display->drawString(0, 20, "Progress:");
      display->drawString(60, 20, "  ");
      display->display();
      display->drawString(60, 20, String((progress / (total / 100))));
      display->display();
    });
  }
  void onError() {
    ArduinoOTA.onError([this](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      display->drawString(0, 40, "Error: ");
      display->display();
      if (error == OTA_AUTH_ERROR) {
        Serial.println("Auth Failed");
        display->drawString(60, 40, "Auth Failed");
        display->display();
      }
      else if (error == OTA_BEGIN_ERROR) {
        Serial.println("Begin Failed");
        display->drawString(60, 40, "Begin Failed");
        display->display();
      }
      else if (error == OTA_CONNECT_ERROR) {
        Serial.println("Connect Failed");
        display->drawString(60, 40, "Connect Failed");
        display->display();
      }
      else if (error == OTA_RECEIVE_ERROR) {
        Serial.println("Receive Failed");
        display->drawString(60, 40, "Receive Failed");
        display->display();
      }
      else if (error == OTA_END_ERROR) {
        Serial.println("End Failed");
        display->drawString(60, 40, "End Failed");
        display->display();
      }
    });
  }
  void check() {
    ArduinoOTA.handle();
    delay(50);
  }
private:
  oledclass* display;
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