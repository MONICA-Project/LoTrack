#include "projectstructs.h"
#include "peripheral.h"
#include "RXTX.hpp"
#include "LED.hpp"
#include "storage.hpp"
typedef LED<pin_led> ledclass;
#include "OLED.hpp"
typedef OLED<pin_oled_sda, pin_oled_scl, pin_oled_pwr, use_display> oledclass;
#include "WLAN.hpp"
typedef WLAN<wifissid, wifipsk, esp_name, telnet_clients, telnet_port, print_over_serialport> wlanclass;
#include "OTA.hpp"
typedef OTA<esp_name> otaclass;
#include "GPS.hpp"
typedef GPS<hardware_serial_id, pin_gps_tx, pin_gps_rx, print_gps_on_serialport> gpsclass;
#include "LORA.hpp"
typedef LORA<pin_lora_miso, pin_lora_mosi, pin_lora_sck, pin_lora_ss, pin_lora_rst, pin_lora_di0, lora_band, esp_name> loraclass;
#include "BATTERY.hpp"
typedef Battery<pin_batt, has_battery> battclass;
#include <pthread.h>

class Program {
public:
  Program() {
    this->s = new RXTX();
    this->led = new ledclass();
    this->batt = new battclass();
    this->wlan = new wlanclass(new oledclass());
    this->aOTA = new otaclass(this->wlan, this->led);
    this->gps = new gpsclass(this->wlan);
    this->storage = new Storage();
    this->lora = new loraclass(this->wlan, this->storage);
  }

  void setup() {
    this->led->on();
    this->wlan->begin();
    this->aOTA->setup();
    this->gps->begin();
    this->storage->begin();
    this->lora->begin();
    this->wlan->box("Create Threads!", 95);
    pthread_mutex_init(&this->mutex_display, NULL);
    pthread_mutex_init(&this->gps->mgp, NULL);
    this->create_gps_thread();
    this->create_wlansniffer_thread();
    this->create_disp_thread();
    this->wlan->box("Init Ok!", 100);
    this->led->off();
  }

  void loop() {
    if(this->loop_thread) {
      pthread_mutex_lock(&this->mutex_display);
      pthread_mutex_lock(&this->gps->mgp);
      this->lora->send(this->gps->getGPSData(), this->batt->getBattery(), false);
      this->led->blink();
      pthread_mutex_unlock(&this->gps->mgp);
      pthread_mutex_unlock(&this->mutex_display);
    } else {
      if(!this->loop_thread_stopped) {
        this->wlan->log("Loop Thread stopped!\n");
        this->loop_thread_stopped = true;
      }
    }
    delay(20000);
  }

  static void *gps_runner(void *obj_class) {
    Program *p = ((Program *)obj_class);
    p->wlan->log("GPS Thread started!\n");
    p->gps->measure();
    p->wlan->log("GPS Thread stopped!\n");
    p->gps_thread_stopped = true;
  }

  static void *wlan_runner(void *obj_class) {
    Program *p = ((Program *)obj_class);
    p->wlan->log(String("WLAN Thread started!\n"));
    uint16_t count = 0;
    bool loop = true;
    while (loop) {
      p->aOTA->check();
      p->wlan->server_clienthandle();
      if(p->wlan->server_has_data()) {
        String command = p->wlan->get_last_string();
        if(command.equals("FREQ")) {
          p->wlan->log("Stopping all other Threads!\n");
          p->gps->stop();
          p->disp_thread = false;
          p->loop_thread = false;
          while(!p->disp_thread_stopped || !p->loop_thread_stopped || !p->gps_thread_stopped) {
            delay(1000);
          }
          int32_t freq = p->storage->readOffsetFreq();
          p->wlan->log("Frequency offset now: " + String(freq) + "\n");
          p->wlan->log("Usage for Frequency offset Mode:\n");
          p->wlan->log("S for Save and Reset, Switch off for not Save!\n");
          p->wlan->log("Any singed integer for tuning: eg. -42 or 1337\n");
          p->lora->debugmode();
          while(true) {
            if(p->wlan->server_has_data()) {
              String r = p->wlan->get_last_string();
              if(r.equals("S")) {
                p->wlan->log("Save " + String(freq) + " as new offset!\n");
                p->storage->writeOffsetFreq(freq);
                p->wlan->log("Reset ESP!\n");
                ESP.restart();
              } else {
                int32_t l = r.toInt();
                if(l != 0) {
                  freq = l;
                  p->wlan->log("Offset: " + String(freq) + "\n");
                  p->lora->setFreqOffset(freq);
                }
              }
            }
            p->lora->send();
            delay(1000);
          }
        }
      }
      if(count > 600) {
        if(p->wlan->getNumClients() != 0) {
          p->wlan->log("Wirless not shut down, Client connected!\n");
        } else {
          p->wlan->log("Wirless shutting down now!\n");
          loop = false;
          p->wlan->stop();
        }
        count = 0;
      } else {
        count++;
      }
      delay(100);
    }
  }

  static void *disp_runner(void *obj_class) {
    Program *p = ((Program *)obj_class);
    p->wlan->log("DISP Thread started!\n");
    while (p->disp_thread) {
      pthread_mutex_lock(&p->mutex_display);
      p->wlan->gps(p->gps->getGPSData(), p->batt->getBattery());
      pthread_mutex_unlock(&p->mutex_display);
      delay(5000);
    }
    p->wlan->log("DISP Thread stopped!\n");
    p->disp_thread_stopped = true;
  }

private:
  RXTX * s;
  otaclass * aOTA;
  gpsclass * gps;
  wlanclass * wlan;
  loraclass * lora;
  ledclass * led;
  battclass * batt;
  Storage * storage;
  pthread_mutex_t mutex_display;
  bool disp_thread = true;
  bool loop_thread = true;
  bool disp_thread_stopped = false;
  bool loop_thread_stopped = false;
  bool gps_thread_stopped = false;
  
  void create_gps_thread() {
    pthread_t thread;
    int return_value = pthread_create(&thread, NULL, &this->gps_runner, this);
    if (return_value) {
      this->wlan->log(String("Failed to Start GPS-Thread!\n"));
    }
  }

  void create_wlansniffer_thread() {
    pthread_t thread;
    int return_value = pthread_create(&thread, NULL, &this->wlan_runner, this);
    if (return_value) {
      this->wlan->log(String("Failed to Start WLAN-Thread!\n"));
    }
  }

  void create_disp_thread() {
    pthread_t thread;
    int return_value = pthread_create(&thread, NULL, &this->disp_runner, this);
    if (return_value) {
      this->wlan->log(String("Failed to Start DISP-Thread!\n"));
    }
  }
};
