#include "peripheral.h"
#include <pthread.h>

void setup () {
  wlan->begin();
  aOTA->setup();
  gps->begin();
  lora->begin();
  display->box("Create Threads!", 95);
  pthread_t thread[3];
  int return_value;
  for (int i = 0; i < 3; i++) {
    return_value = pthread_create(&thread[i], NULL, printThreadId, (void *)i);
    if (return_value) {
      Serial.println("An error has occurred");
    }
  }
  display->box("Init Ok!", 100);
}

void loop () {
  aOTA->check();
  display->wifi(wlan->getWifiData(), wlan->size);
  delay(1000);
  display->gps(gps->getGPSData());
  delay(1000);
}

void *printThreadId(void *threadid) {
  if ((int)threadid == 0) {
    gps_runner();
  } else if ((int)threadid == 1) {
    wlan_runner();
  } else if ((int)threadid == 2) {
    send_runner();
  }
}

void gps_runner() {
  Serial.println("GPS Runner started!");
  gps->measure();
}

void wlan_runner() {
  Serial.println("WLAN Runner started!");
  while (true) {
    wlan->measure();
    delay(100);
  }
}

void send_runner() {
  while (true) {
    lora->send(wlan->getWifiData(), gps->getGPSData(), wlan->size);
    led->blink();
    delay(100);
  }
}