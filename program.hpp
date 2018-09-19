#include "projectstructs.h"
#include "peripheral.h"
#include "RXTX.hpp"
#include "LED.hpp"
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
    this->lora = new loraclass(this->wlan);
  }

  void setup() {
    this->led->on();
    this->wlan->begin();
    this->aOTA->setup();
    this->gps->begin();
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
    pthread_mutex_lock(&this->mutex_display);
    this->wlan->gps(this->gps->getGPSData(), this->batt->getBattery());
    pthread_mutex_unlock(&this->mutex_display);
    delay(5000);
  }

  static void *gps_runner(void *obj_class) {
    Program *p = ((Program *)obj_class);
    p->wlan->log(String("GPS Runner started!\n"));
    p->gps->measure();
  }

  static void *wlan_runner(void *obj_class) {
    Program *p = ((Program *)obj_class);
    p->wlan->log(String("WLAN Runner started!\n"));
    uint16_t count = 0;
    bool loop = true;
    while (loop) {
      p->aOTA->check();
      p->wlan->server_clienthandle();
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
    p->wlan->log(String("LORA Runner started!\n"));
    while (true) {
      pthread_mutex_lock(&p->mutex_display);
      pthread_mutex_lock(&p->gps->mgp);
      p->lora->send(p->gps->getGPSData(), p->batt->getBattery(), false);
      p->led->blink();
      pthread_mutex_unlock(&p->gps->mgp);
      pthread_mutex_unlock(&p->mutex_display);
      delay(20000);
    }
  }

private:
  RXTX * s;
  otaclass * aOTA;
  gpsclass * gps;
  wlanclass * wlan;
  loraclass * lora;
  ledclass * led;
  battclass * batt;
  pthread_mutex_t mutex_display;
  
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
