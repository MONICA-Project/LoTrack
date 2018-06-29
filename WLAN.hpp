#include <WiFi.h>
#include <mutex>

template<const char* ssid, const char* psk_key, const char* espname>
class WLAN {
public:
  const static uint8_t size = 10;
  WLAN(oledclass* disp) {
    this->display = disp;
  }
  void begin() {
    this->display->box("Setup Wifi!", 20);
    this->w = new WiFiClass();
    this->w->mode(WIFI_STA);
    this->w->setHostname(espname);
    WiFi.begin(ssid, psk_key);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
      this->display->box("Not connected to WiFi", 30);
    }
    else {
      this->display->box("WiFi connected: " + String(ssid), 30);
    }
  }
  void measure() {
    uint8_t n = this->w->scanNetworks();
    String bssid;
    if (n == 0) {
      this->mtx.lock();
      for (uint8_t i = 0; i < this->size; i++) {
        this->data[i].mac_Address = "000000000000";
        this->data[i].mac_RSSI = "0000";
        this->data[i].mac_Channel = "00";
        this->data[i].mac_SSID = "";
      }
      this->mtx.unlock();
    } else {
      //Serial.println(String("Networks found: ") + String(n));
      int8_t indices[n];
      for (int i = 0; i < n; i++) {
        indices[i] = i;
      }
      for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
          if (WiFi.RSSI(indices[j]) > WiFi.RSSI(indices[i])) {
            std::swap(indices[i], indices[j]);
          }
        }
      }
      for (int i = 0; i < n; i++) {
        bssid = WiFi.SSID(indices[i]);
        for (int j = i + 1; j < n; j++) {
          if (bssid == WiFi.SSID(indices[j])) {
            indices[j] = -1;
          }
        }
      }
      this->mtx.lock();
      this->networks = n;
      if (n > this->size) {
        n = this->size;
      }
      for (uint8_t i = 0; i < this->size; i++) {
        if (i < n && indices[i] >= 0) {
          String bssid = WiFi.BSSIDstr(indices[i]);
          bssid.replace(":", "");
          this->data[i].mac_Address = bssid;
          this->data[i].mac_RSSI = String(WiFi.RSSI(indices[i]));
          this->data[i].mac_Channel = String(WiFi.channel(indices[i]));
          this->data[i].mac_SSID = String(WiFi.SSID(indices[i]));
        } else {
          this->data[i].mac_Address = "000000000000";
          this->data[i].mac_RSSI = "0000";
          this->data[i].mac_Channel = "00";
          this->data[i].mac_SSID = "";
        }
      }
      this->mtx.unlock();
    }
  }
  void show()
  {
    this->mtx.lock();
    this->display->clear();
    this->display->drawString(0, 0, "Networks found: " + String(this->networks));
    Serial.println("################################################");
    Serial.println("Networks found: " + String(this->networks));
    for (int i = 0; i < size; i++) {
      this->display->drawString(0, ((i * 10) + 10), this->data[i].mac_Address);
      this->display->drawString(100, ((i * 10) + 10), this->data[i].mac_RSSI);
      Serial.println(this->data[i].mac_Address + " " + this->data[i].mac_RSSI + " " + this->data[i].mac_Channel + " (" + this->data[i].mac_SSID + ")");
    }
    this->mtx.unlock();
    this->display->display();
  }
  void lock() {
    this->mtx.lock();
  }
  void unlock() {
    this->mtx.unlock();
  }
  macInfoField *data = new macInfoField[this->size];
  uint8_t networks = 0;
private:
  WiFiClass * w = NULL;
  oledclass* display;
  std::mutex mtx;
};