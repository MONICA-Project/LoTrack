#define NO_GLOBAL_ARDUINOOTA true
#include <ArduinoOTA.h>

template<const char* espname>
class OTA {
  public:
    bool isRunning = false;
    OTA(wlanclass* wlanclass, ledclass * ledclass) {
      this->wlan = wlanclass;
      this->led = ledclass;
      this->otaClass = new ArduinoOTAClass();
    }

    void Begin() {
      this->wlan->Box("OTA Setup!", 40);
      this->OnStart();
      this->OnEnd();
      this->OnProgress();
      this->OnError();
      this->otaClass->setHostname(espname);
      this->otaClass->begin();
      this->wlan->Box("OTA Successfull", 50);
    }

    void OnStart() {
      this->otaClass->onStart([this]() {
        this->isRunning = true;
        String type;
        if(this->otaClass->getCommand() == U_FLASH) {
          type = "sketch";
        } else {
          type = "filesystem"; // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
        }
        this->wlan->Clear();
        this->wlan->DrawString(0, 0, "Start updating:" + type);
        this->wlan->Display();
      });
    }

    void OnEnd() {
      this->otaClass->onEnd([this]() {
        this->wlan->DrawString(0, 40, "End");
        this->wlan->Display();
      });
    }

    void OnProgress() {
      this->blink = 0;
      this->otaClass->onProgress([this](uint32_t progress, uint32_t total) {
        float p = ((float)progress / total)*100;
        if(this->blink++ % 2) {
          this->led->Color(this->led->BLUE);
        } else {
          this->led->Color(this->led->BLACK);
        }
        this->wlan->Log(String(p, 2) + String(" "));
        this->wlan->Box(String("Progess update..."), (uint8_t)p);
      });
    }

    void OnError() {
      this->otaClass->onError([this](ota_error_t error) {
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
      this->otaClass->handle();
    }

  private:
    wlanclass * wlan;
    ledclass * led;
    uint32_t blink;
    ArduinoOTAClass * otaClass = NULL;
};
