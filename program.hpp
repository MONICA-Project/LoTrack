#include "projectstructs.h"
#include "peripheral.h"
#include "RXTX.hpp"
#include "LED.hpp"
#include "STORAGE.hpp"

typedef LED<pin_ledr, pin_ledg, pin_ledb> ledclass;
#include "OLED.hpp"
typedef OLED<pin_oled_sda, pin_oled_scl, pin_oled_pwr> oledclass;
#include "WLAN.hpp"
typedef WLAN<wifissid, wifipsk, esp_name, 2, 23, print_over_serialport> wlanclass;
#include "OTA.hpp"
typedef OTA<esp_name> otaclass;
#include "GPS.hpp"
typedef GPS<pin_gps_tx, pin_gps_rx, pin_enable_gnss, print_gps_on_serialport> gpsclass;
#include "LORA.hpp"
typedef LORA<pin_lora_miso, pin_lora_mosi, pin_lora_sck, pin_lora_ss, pin_lora_rst, pin_lora_di0, lora_band, esp_name, listenbeforetalk, lora_send_binary> loraclass;
#include "BATTERY.hpp"
typedef Battery<pin_batt> battclass;
#include "SLEEP.hpp"
typedef Sleep<pin_regulator_enable, pin_button> sleepclass;
#include <pthread.h>

/// <summary>Global program class, that execute the whole program</summary>
class Program {
  public:
    /// <summary>Constructor of programm class, init all subclasses</summary>
    Program() {
      this->s = new RXTX();
      this->led = new ledclass();
      this->sleep = new sleepclass(this->led);
      this->wlan = new wlanclass(new oledclass());
      this->aOTA = new otaclass(this->wlan, this->led);
      this->gps = new gpsclass(this->wlan);
      this->storage = new Storage();
      this->batt = new battclass(this->storage);
      this->lora = new loraclass(this->wlan, this->storage);
    }

    /// <summary>Startup the controller, init wifi if controller was resetted, otherwise (start from sleep mode) not</summary>
    void Begin() {
      this->led->Color(this->led->RED);
      uint8_t sleepReason = this->sleep->GetWakeupReason();
      this->wlan->Begin();
      if(sleepReason == 0) {
        this->wlan->Connect();
        this->aOTA->Begin();
      } else {
        this->wlan->Stop();
      }
      this->gps->Begin();
      this->storage->Begin();
      this->lora->Begin();
      pthread_mutex_init(&this->sleep->MutexSleep, NULL);
      pthread_mutex_init(&this->mutexDisplay, NULL);
      this->CreateButtonThread();
      this->sleep->AttachInterrupt(this->IsrButtonRoutine, this);
      if(sleepReason == 0) {
        this->wlan->Box("Create Threads!", 95);
      }
      this->CreateGpsThread();
      if(sleepReason == 0) {
        this->CreateWlanThread();
        this->CreateDispThread();
        this->wlan->Box("Init Ok!", 100);
      }
      this->led->Color(this->led->BLACK);
      if(sleepReason == 0) {
        this->sendStartupInfos = true;
      } else {
        this->sendStartupInfos = false;
        this->sleep->EnableSleep();
      }
    }

    /// <summary>Mainloop, looking for gps, sended over lora</summary>
    void Loop() {
      if(this->sendStartupInfos) {
        this->sendStartupInfos = false;
        this->lora->Send(this->version, this->wlan->GetIp(), this->wlan->GetSsid(), this->wlan->GetStatus(), this->batt->GetBattery(), this->storage->ReadOffsetFreq(), 1);
      }
      if(this->loopThread) {
        if(this->wlan->GetStatus()) {
          this->aOTA->Check();
        }
        while(!this->gps->HasData()) {
          delay(100);
        }
        pthread_mutex_lock(&this->mutexDisplay);
        pthread_mutex_lock(&this->gps->MutexGps);
        gpsInfoField g = this->gps->GetGPSData();
        this->lora->Send(g, this->batt->GetBattery());
        if(g.fix) {
          this->led->Blink(this->led->YELLOW);
          delay(100);
          this->led->Blink(this->led->YELLOW);
        } else {
          this->led->Blink(this->led->RED);
        }
        pthread_mutex_unlock(&this->gps->MutexGps);
        pthread_mutex_unlock(&this->mutexDisplay);
      } else {
        if(!this->loopThreadStopped) {
          this->wlan->Log("Loop Thread stopped!\n");
          this->loopThreadStopped = true;
        }
      }
      this->sleep->TimerSleep();
    }
  private:
    const uint8_t version = 13;
    /**
     * 1 Refactoring and Send networksettings over lora
     * 2 Sleepmode and Powersaving implemented
     * 3 Add height to Lora transmission
     * 4 Looking if Lorachannel is free (ListenBeforeTalk)
     * 5 Option for LBT, also 5s sleep time, CR to 5, SF to 9 and BW to 125000, fixing a parsing bug for GPS, change the Transmitpower to 20
     * 6 Create new Binary Version
     * 7 Added GNSS_Enable Pin, RGB LED Support
     * 8 Added Device_Enable Pin
     * 9 Added Button support for shutting down the device on long press, also for short press sending the location as emergency
     * 10 When Shutting down the Device, Send a Lora Status message. Send Panic Message 3 Times with different SF Settings
     * 11 OTA Update now in mainthread because of stacksize to small in pthread and displaying the MAC address in the serial log
     * 12 Add a primitive mutex, so that an corrupted esp not create tons of button threads and the controller crashs, also change led behavour
     * 13 Add internal programmable offset for Battery
     */
    RXTX * s;
    otaclass * aOTA;
    gpsclass * gps;
    wlanclass * wlan;
    loraclass * lora;
    ledclass * led;
    battclass * batt;
    Storage * storage;
    sleepclass * sleep;
  
    pthread_mutex_t mutexDisplay;
    bool dispThread = true;
    bool loopThread = true;
    bool dispThreadStopped = false;
    bool loopThreadStopped = false;
    bool gpsThreadStopped = false;
    bool sendStartupInfos = false;
    volatile bool buttonIsRunning = false;
  
    void CreateGpsThread() {
      pthread_t thread;
      int return_value = pthread_create(&thread, NULL, &this->GpsRunner, this);
      if (return_value) {
        this->wlan->Log(String("Failed to Start GPS-Thread!\n"));
      }
    }

    static void *GpsRunner(void *obj_class) {
      Program *p = ((Program *)obj_class);
      p->wlan->Log("GPS Thread started!\n");
      p->gps->Measure();
      p->wlan->Log("GPS Thread stopped!\n");
      p->gpsThreadStopped = true;
    }

    void CreateWlanThread() {
      pthread_t thread;
      int return_value = pthread_create(&thread, NULL, &this->WlanRunner, this);
      if (return_value) {
        this->wlan->Log(String("Failed to Start WLAN-Thread!\n"));
      }
    }

    static void *WlanRunner(void *obj_class) {
      Program *p = ((Program *)obj_class);
      p->wlan->Log(String("WLAN Thread started!\n"));
      uint16_t count = 0;
      bool loop = true;
      while (loop) {
        p->wlan->ServerClienthandle();
        if (p->wlan->ServerHasData()) {
          String command = p->wlan->GetLastString();
          if (command.equals("FREQ")) {
            p->wlan->Log("Stopping all other Threads!\n");
            p->gps->Stop();
            p->dispThread = false;
            p->loopThread = false;
            while (!p->dispThreadStopped || !p->loopThreadStopped || !p->gpsThreadStopped) {
              delay(1000);
            }
            int32_t freq = p->storage->ReadOffsetFreq();
            p->wlan->Log("Target frequency is: " + String(lora_band) + "\n");
            p->wlan->Log("Frequency offset now: " + String(freq) + "\n");
            p->wlan->Log("Usage for Frequency offset Mode:\n");
            p->wlan->Log("S for Save and Reset, Switch off for not Save!\n");
            p->wlan->Log("Any singed integer for tuning: eg. -42 or 1337\n");
            p->lora->Debugmode();
            while (true) {
              if (p->wlan->ServerHasData()) {
                String r = p->wlan->GetLastString();
                if (r.equals("S")) {
                  p->wlan->Log("Save " + String(freq) + " as new offset!\n");
                  p->storage->WriteOffsetFreq(freq);
                  p->wlan->Log("Reset ESP!\n");
                  ESP.restart();
                } else if (r.equals("Q")) {
                  p->wlan->Log("Reset ESP!\n");
                  ESP.restart();
                } else {
                  int32_t l = r.toInt();
                  if (l != 0) {
                    freq = l;
                    p->wlan->Log("Offset: " + String(freq) + "\n");
                    p->lora->SetFreqOffset(freq);
                  }
                }
              }
              p->lora->DebugSend();
              delay(1000);
            }
          } else if (command.equals("BATT")) {
            float_t batt = p->storage->ReadBatteryOffset();
            p->wlan->Log("Battery offset now: " + String(batt,2) + "\n");
            p->wlan->Log("Usage for Battery offset Mode:\n");
            p->wlan->Log("S for Save and Reset, Q for Quit and not Save!\n");
            p->wlan->Log("Any singed float for tuning: eg. -0.42 or 0.1337\n");
            while (true) {
              if (p->wlan->ServerHasData()) {
                String r = p->wlan->GetLastString();
                if (r.equals("S")) {
                  p->wlan->Log("Save " + String(batt, 2) + " as new offset!\n");
                  p->storage->WriteBatteryOffset(batt);
                  p->wlan->Log("Reset ESP!\n");
                  ESP.restart();
                } else if (r.equals("Q")) {
                  p->wlan->Log("Reset ESP!\n");
                  ESP.restart();
                } else {
                  float_t l = r.toFloat();
                  if (!isnan(l)) {
                    batt = l;
                    p->wlan->Log("Offset: " + String(batt, 2) + "\n");
                    p->batt->SetOffset(batt);
                  }
                }
              }
              delay(1000);
            }
          }
        }
        if (count > 600) {
          if (p->wlan->GetNumClients() != 0 || p->aOTA->isRunning) {
            p->wlan->Log("Wirless not shut down, Client connected!\n");
          }
          else {
            p->wlan->Log("Wireless shutting down now!\n");
            loop = false;
            p->wlan->Stop();
            p->dispThread = false;
            p->lora->Send(p->version, p->wlan->GetIp(), p->wlan->GetSsid(), p->wlan->GetStatus(), p->batt->GetBattery(), p->storage->ReadOffsetFreq(), 2);
            p->sleep->EnableSleep();
          }
          count = 0;
        }
        else {
          count++;
        }
        delay(100);
      }
    }

    void CreateDispThread() {
      pthread_t thread;
      int return_value = pthread_create(&thread, NULL, &this->DispRunner, this);
      if (return_value) {
        this->wlan->Log(String("Failed to Start DISP-Thread!\n"));
      }
    }

    static void *DispRunner(void *obj_class) {
      Program *p = ((Program *)obj_class);
      p->wlan->Log("DISP Thread started!\n");
      while (p->dispThread) {
        pthread_mutex_lock(&p->mutexDisplay);
        p->wlan->Gps(p->gps->GetGPSData(), p->batt->GetBattery());
        pthread_mutex_unlock(&p->mutexDisplay);
        delay(5000);
      }
      p->wlan->Log("DISP Thread stopped!\n");
      p->dispThreadStopped = true;
    }

    static void IsrButtonRoutine(void * obj_class) {
      Program *p = ((Program *)obj_class);
      p->CreateButtonThread();
    }

    void CreateButtonThread() {
      if(this->buttonIsRunning) {
        return;
      }
      this->buttonIsRunning = true;
      pthread_t thread;
      int return_value = pthread_create(&thread, NULL, &this->ButtonRunner, this);
      if(return_value) {
        this->wlan->Log(String("Failed to Start BUTTON-Thread!\n"));
      }
    }

    static void *ButtonRunner(void *obj_class) {
      Program *p = ((Program *)obj_class);
      p->wlan->Log(String("Start Button Thread\n"));
      pthread_mutex_lock(&p->sleep->MutexSleep);
      uint8_t task = p->sleep->GetButtonMode();
      if (task == 2) {
        p->wlan->Log(String("SHUTDOWN!\n"));
        p->lora->Send(p->version, p->wlan->GetIp(), p->wlan->GetSsid(), p->wlan->GetStatus(), p->batt->GetBattery(), p->storage->ReadOffsetFreq(), 0);
        p->sleep->Shutdown();
      }
      if (task == 1) {
        p->wlan->Log(String("PANIC Mode Send!\n"));
        for(uint8_t i = 0; i < 5; i++) {
          p->led->Blink(p->led->YELLOW);
          delay(100);
        }
        p->led->Color(p->led->YELLOW);
        while (!p->gps->HasData()) {
          delay(100);
        }
        pthread_mutex_lock(&p->mutexDisplay);
        pthread_mutex_lock(&p->gps->MutexGps);
        gpsInfoField g = p->gps->GetGPSData();
        p->lora->Send(g, p->batt->GetBattery(), true);
        p->led->Color(p->led->BLACK);
        pthread_mutex_unlock(&p->gps->MutexGps);
        pthread_mutex_unlock(&p->mutexDisplay);
      }
      pthread_mutex_unlock(&p->sleep->MutexSleep);
      p->buttonIsRunning = false;
    }
};
