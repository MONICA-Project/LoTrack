#include "projectstructs.h"
#include "peripheral.h"
#include "RXTX.hpp"
#include "LED.hpp"
#include "STORAGE.hpp"


typedef LED<pin_ledr, pin_ledg, pin_ledb> ledclass;
#include "OLED.hpp"
typedef OLED<pin_oled_sda, pin_oled_scl, pin_oled_pwr> oledclass;
#include "WLAN.hpp"
typedef WLAN<wifissid, wifipsk, esp_name, telnet_clients, telnet_port, print_over_serialport> wlanclass;
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

class Program {
public:
  Program() {
    this->s = new RXTX();
    this->led = new ledclass();
    this->sleep = new sleepclass(this->led);
    this->batt = new battclass();
    this->wlan = new wlanclass(new oledclass());
    this->aOTA = new otaclass(this->wlan, this->led);
    this->gps = new gpsclass(this->wlan);
    this->storage = new Storage();
    this->lora = new loraclass(this->wlan, this->storage);
  }

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
    this->sleep->AttachInterrupt(this->abc, this);
    if(sleepReason == 0) {
      this->wlan->Box("Create Threads!", 95);
    }
    pthread_mutex_init(&this->mutex_display, NULL);
    this->CreateGpsThread();
    if(sleepReason == 0) {
      this->CreateWlanThread();
      this->CreateDispThread();
      this->wlan->Box("Init Ok!", 100);
    }
    this->led->Color(this->led->BLACK);
    if(sleepReason == 0) {
      this->send_startup_infos = true;
    } else {
      this->send_startup_infos = false;
      this->sleep->EnableSleep();
    }
  }
  
  static void abc(void * obj_class) {
    Program *p = ((Program *)obj_class);
    if(p->sleep->ButtonPressed()) {
      p->wlan->Log(String("Begin Getting Task\n"));
      uint8_t task = p->sleep->GetButtonMode();
      /*if(task == 2) {
        p->sleep->Shutdown();
      }*/
      p->wlan->Log(String("Task: ")+String(task)+String("\n"));
    }
  }

  void Loop() {
    /*if(this->sleep->ButtonPressed()) {
      this->wlan->Log(String("Begin Getting Task"));
      uint8_t task = this->sleep->GetButtonMode();
      if(task == 2) {
        this->sleep->Shutdown();
      }
      this->wlan->Log(String("Task: ")+String(task));
    }*/
    if(this->send_startup_infos) {
      this->send_startup_infos = false;
      this->lora->Send(this->version, this->wlan->GetIp(), this->wlan->GetSsid(), this->wlan->GetStatus(), this->batt->GetBattery(), this->storage->ReadOffsetFreq());
    }
    if(this->loop_thread) {
      while(!this->gps->HasData()) {
        delay(100);
      }
      pthread_mutex_lock(&this->mutex_display);
      pthread_mutex_lock(&this->gps->MutexGps);
      gpsInfoField g = this->gps->GetGPSData();
      this->lora->Send(g, this->batt->GetBattery());
      if(g.fix) {
        this->led->Blink(this->led->YELLOW);
      } else {
        this->led->Blink(this->led->RED);
      }
      pthread_mutex_unlock(&this->gps->MutexGps);
      pthread_mutex_unlock(&this->mutex_display);
    } else {
      if(!this->loop_thread_stopped) {
        this->wlan->Log("Loop Thread stopped!\n");
        this->loop_thread_stopped = true;
      }
    }
    this->sleep->TimerSleep();
  }

  

  static void *GpsRunner(void *obj_class) {
    Program *p = ((Program *)obj_class);
    p->wlan->Log("GPS Thread started!\n");
    p->gps->Measure();
    p->wlan->Log("GPS Thread stopped!\n");
    p->gps_thread_stopped = true;
  }

  static void *WlanRunner(void *obj_class) {
    Program *p = ((Program *)obj_class);
    p->wlan->Log(String("WLAN Thread started!\n"));
    uint16_t count = 0;
    bool loop = true;
    while (loop) {
      p->aOTA->Check();
      p->wlan->ServerClienthandle();
      if(p->wlan->ServerHasData()) {
        String command = p->wlan->GetLastString();
        if(command.equals("FREQ")) {
          p->wlan->Log("Stopping all other Threads!\n");
          p->gps->Stop();
          p->disp_thread = false;
          p->loop_thread = false;
          while(!p->disp_thread_stopped || !p->loop_thread_stopped || !p->gps_thread_stopped) {
            delay(1000);
          }
          int32_t freq = p->storage->ReadOffsetFreq();
          p->wlan->Log("Target frequency is: " + String(lora_band) + "\n");
          p->wlan->Log("Frequency offset now: " + String(freq) + "\n");
          p->wlan->Log("Usage for Frequency offset Mode:\n");
          p->wlan->Log("S for Save and Reset, Switch off for not Save!\n");
          p->wlan->Log("Any singed integer for tuning: eg. -42 or 1337\n");
          p->lora->Debugmode();
          while(true) {
            if(p->wlan->ServerHasData()) {
              String r = p->wlan->GetLastString();
              if(r.equals("S")) {
                p->wlan->Log("Save " + String(freq) + " as new offset!\n");
                p->storage->WriteOffsetFreq(freq);
                p->wlan->Log("Reset ESP!\n");
                ESP.restart();
              } else {
                int32_t l = r.toInt();
                if(l != 0) {
                  freq = l;
                  p->wlan->Log("Offset: " + String(freq) + "\n");
                  p->lora->SetFreqOffset(freq);
                }
              }
            }
            p->lora->DebugSend();
            delay(1000);
          }
        }
      }
      if(count > 600) {
        if(p->wlan->GetNumClients() != 0) {
          p->wlan->Log("Wirless not shut down, Client connected!\n");
        } else {
          p->wlan->Log("Wireless shutting down now!\n");
          loop = false;
          p->wlan->Stop();
          p->disp_thread = false;
          p->lora->Send(p->version, p->wlan->GetIp(), p->wlan->GetSsid(), p->wlan->GetStatus(), p->batt->GetBattery(), p->storage->ReadOffsetFreq());
          p->sleep->EnableSleep();
        }
        count = 0;
      } else {
        count++;
      }
      delay(100);
    }
  }

  static void *DispRunner(void *obj_class) {
    Program *p = ((Program *)obj_class);
    p->wlan->Log("DISP Thread started!\n");
    while (p->disp_thread) {
      pthread_mutex_lock(&p->mutex_display);
      p->wlan->Gps(p->gps->GetGPSData(), p->batt->GetBattery());
      pthread_mutex_unlock(&p->mutex_display);
      delay(5000);
    }
    p->wlan->Log("DISP Thread stopped!\n");
    p->disp_thread_stopped = true;
  }

private:
  const uint8_t version = 9;
  /**
   * 1 Refactoring and Send networksettings over lora
   * 2 Sleepmode and Powersaving implemented
   * 3 Add height to Lora transmission
   * 4 Looking if Lorachannel is free (ListenBeforeTalk)
   * 5 Option for LBT, also 5s sleep time, CR to 5, SF to 9 and BW to 125000, fixing a parsing bug for GPS, change the Transmitpower to 20
   * 6 Create new Binary Version
   * 7 Added GNSS_Enable Pin, RGB LED Support
   * 8 Added Device_Enable Pin
   * 9 Merge Button + Sleep together, because of sleepmodes, interrupts and wakeups
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
  
  pthread_mutex_t mutex_display;
  bool disp_thread = true;
  bool loop_thread = true;
  bool disp_thread_stopped = false;
  bool loop_thread_stopped = false;
  bool gps_thread_stopped = false;
  bool send_startup_infos = false;
  
  void CreateGpsThread() {
    pthread_t thread;
    int return_value = pthread_create(&thread, NULL, &this->GpsRunner, this);
    if (return_value) {
      this->wlan->Log(String("Failed to Start GPS-Thread!\n"));
    }
  }

  void CreateWlanThread() {
    pthread_t thread;
    int return_value = pthread_create(&thread, NULL, &this->WlanRunner, this);
    if (return_value) {
      this->wlan->Log(String("Failed to Start WLAN-Thread!\n"));
    }
  }

  void CreateDispThread() {
    pthread_t thread;
    int return_value = pthread_create(&thread, NULL, &this->DispRunner, this);
    if (return_value) {
      this->wlan->Log(String("Failed to Start DISP-Thread!\n"));
    }
  }
};
