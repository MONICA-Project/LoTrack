#include <ArduinoOTA.h>

template<const char* espname>
class OTA {
public:
  OTA(wlanclass* wlanclass, ledclass * ledclass) {
    this->wlan = wlanclass;
    this->led = ledclass;
  }
  void Begin() {
    this->wlan->Box("OTA Setup!", 40);
    this->OnStart();
    this->OnEnd();
    this->OnProgress();
    this->OnError();
    ArduinoOTA.setHostname(espname);
    ArduinoOTA.begin();
    this->wlan->Box("OTA Successfull", 50);
  }
  void OnStart() {
    ArduinoOTA.onStart([this]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";
      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      this->wlan->Clear();
      this->wlan->DrawString(0, 0, "Start updating:" + type);
      this->wlan->Display();
    });
  }
  void OnEnd() {
    ArduinoOTA.onEnd([this]() {
      this->wlan->DrawString(0, 40, "End");
      this->wlan->Display();
    });
  }
  void OnProgress() {
    ArduinoOTA.onProgress([this](unsigned int progress, unsigned int total) {
      uint8_t p = progress / (total / 100);
      if(p % 2) {
        led->On('b');
      } else {
        led->Off('b');
      }
      this->wlan->Log(String(p) + String(" "));
      this->wlan->Box(String("Progess update..."), p);
    });
  }
  void OnError() {
    ArduinoOTA.onError([this](ota_error_t error) {
      this->wlan->DrawString(0, 40, "Error: ");
      this->wlan->Display();
      if (error == OTA_AUTH_ERROR) {
        this->wlan->DrawString(60, 40, "Auth Failed");
        this->wlan->Display();
      }
      else if (error == OTA_BEGIN_ERROR) {
        this->wlan->DrawString(60, 40, "Begin Failed");
        this->wlan->Display();
      }
      else if (error == OTA_CONNECT_ERROR) {
        this->wlan->DrawString(60, 40, "Connect Failed");
        this->wlan->Display();
      }
      else if (error == OTA_RECEIVE_ERROR) {
        this->wlan->DrawString(60, 40, "Receive Failed");
        this->wlan->Display();
      }
      else if (error == OTA_END_ERROR) {
        this->wlan->DrawString(60, 40, "End Failed");
        this->wlan->Display();
      }
    });
  }
  void Check() {
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
