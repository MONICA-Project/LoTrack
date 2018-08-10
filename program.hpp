#include "peripheral.h"
#include <pthread.h>

class Program {
public:
  Program() {
    this->s = new RXTX();
    this->display = new oledclass();
    this->aOTA = new OTA(display);
    this->gps = new gpsclass(display);
    this->wlan = new wlanclass(display);
    this->lora = new loraclass(display);
    this->led = new ledclass();
  }

  void setup() {
    this->wlan->begin();
    this->aOTA->setup();
    this->gps->begin();
    this->lora->begin();
    this->display->box("Create Threads!", 95);
    pthread_mutex_init(&this->mdp, NULL);
    pthread_mutex_init(&this->mwl, NULL);
    pthread_mutex_init(&this->gps->mgp, NULL);
    this->create_gps_thread();
    //this->create_wlansniffer_thread();
    this->create_lora_thread();
    this->display->box("Init Ok!", 100);
  }

  void loop() {
    pthread_mutex_lock(&this->mdp);
    this->aOTA->check();
    this->display->gps(this->gps->getGPSData(), this->wlan->getNetworks());
    //delay(500);
    //this->display->clear();
    //this->display->display();
    pthread_mutex_unlock(&this->mdp);
    delay(1000);
  }

  static void *gps_runner(void *obj_class) {
    Program *p = ((Program *)obj_class);
    //Serial.println("GPS Runner started!");
    p->gps->measure();
  }

  static void *wlan_runner(void *obj_class) {
    Program *p = ((Program *)obj_class);
    //Serial.println("WLAN Runner started!");
    while (true) {
      pthread_mutex_lock(&p->mwl);
      p->wlan->measure();
      pthread_mutex_unlock(&p->mwl);
      delay(1000);
    }
  }

  static void *lora_runner(void *obj_class) {
    Program *p = ((Program *)obj_class);
    //Serial.println("LORA Runner started!");
    while (true) {
      pthread_mutex_lock(&p->mdp);
      pthread_mutex_lock(&p->mwl);
      pthread_mutex_lock(&p->gps->mgp);
      p->lora->send(p->wlan, p->gps->getGPSData());
      p->led->blink();
      pthread_mutex_unlock(&p->gps->mgp);
      pthread_mutex_unlock(&p->mwl);
      pthread_mutex_unlock(&p->mdp);
      delay(5000);
    }
  }

private:
  RXTX * s;
  oledclass* display;
  OTA* aOTA;
  gpsclass* gps;
  wlanclass* wlan;
  loraclass* lora;
  ledclass* led;
  pthread_mutex_t mdp;
  pthread_mutex_t mwl;
  
  void create_gps_thread() {
    pthread_t thread;
    int return_value = pthread_create(&thread, NULL, &this->gps_runner, this);
    if (return_value) {
      Serial.println("Failed to Start GPS-Runner");
    }
  }

  void create_wlansniffer_thread() {
    pthread_t thread;
    int return_value = pthread_create(&thread, NULL, &this->wlan_runner, this);
    if (return_value) {
      Serial.println("Failed to Start WLAN-Runner");
    }
  }

  void create_lora_thread() {
    pthread_t thread;
    int return_value = pthread_create(&thread, NULL, &this->lora_runner, this);
    if (return_value) {
      Serial.println("Failed to Start LORA-Runner");
    }
  }
};
