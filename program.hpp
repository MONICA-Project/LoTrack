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
    this->create_lora_thread();
    this->wlan->box("Init Ok!", 100);
    this->led->off();
  }

  void loop() {
    if(this->loop_thread) {
      pthread_mutex_lock(&this->mutex_display);
      this->wlan->gps(this->gps->getGPSData(), this->batt->getBattery());
      pthread_mutex_unlock(&this->mutex_display);
    } else {
      if(!this->loop_thread_stopped) {
        this->wlan->log("Loop Thread stopped!\n");
        this->loop_thread_stopped = true;
      }
    }
    delay(5000);
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
          p->lora_thread = false;
          p->loop_thread = false;
          while(!p->lora_thread_stopped || !p->loop_thread_stopped || !p->gps_thread_stopped) {
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

  static void *lora_runner(void *obj_class) {
    Program *p = ((Program *)obj_class);
    p->wlan->log("LORA Thread started!\n");
    while (p->lora_thread) {
      pthread_mutex_lock(&p->mutex_display);
      pthread_mutex_lock(&p->gps->mgp);
      p->lora->send(p->gps->getGPSData(), p->batt->getBattery(), false);
      p->led->blink();
      pthread_mutex_unlock(&p->gps->mgp);
      pthread_mutex_unlock(&p->mutex_display);
      delay(20000);
    }
    p->wlan->log("LORA Thread stopped!\n");
    p->lora_thread_stopped = true;
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
  bool lora_thread = true;
  bool loop_thread = true;
  bool lora_thread_stopped = false;
  bool loop_thread_stopped = false;
  bool gps_thread_stopped = false;
  
  void create_gps_thread() {
    pthread_t thread;
    int return_value = pthread_create(&thread, NULL, &this->gps_runner, this);
    if (return_value) {
      this->wlan->log(String("Failed to Start GPS-Runner!\n"));
    }
  }

  void create_wlansniffer_thread() {
    pthread_t thread;
    int return_value = pthread_create(&thread, NULL, &this->wlan_runner, this);
    if (return_value) {
      this->wlan->log(String("Failed to Start WLAN-Runner!\n"));
    }
  }

  void create_lora_thread() {
    pthread_t thread;
    int return_value = pthread_create(&thread, NULL, &this->lora_runner, this);
    if (return_value) {
      this->wlan->log(String("Failed to Start LORA-Runner!\n"));
    }
  }
};
