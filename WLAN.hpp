#include <WiFi.h>
#include <mutex>

template<const char* ssid, const char* psk_key, const char* espname>
class WLAN {
public:
  WLAN(oledclass* disp) {
    this->display = disp;
  }
  void begin() {
    display->box("Setup Wifi!", 20);
    this->w = new WiFiClass();
    this->w->mode(WIFI_STA);
    this->w->setHostname(espname);
    WiFi.begin(ssid, psk_key);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
      display->box("Not connected to WiFi", 30);
    }
    else {
      display->box("WiFi connected: " + String(ssid), 30);
    }
  }
  void measure() {
    uint8_t n = this->w->scanNetworks();
    String bssid;
    if (n == 0) {
      //Serial.println("No Network Found!");
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
      uint8_t o = n;
      for (int i = 0; i < n; i++) {
        bssid = WiFi.SSID(indices[i]);
        for (int j = i + 1; j < n; j++) {
          if (bssid == WiFi.SSID(indices[j])) {
            indices[j] = -1;
          }
        }
        if (indices[i] < 0) {
          --o;
        }
      }
      mtx.lock();
      this->data = new macInfoField[o];
      this->data_size = o;
      for (int i = 0; i < o; ++i) {
        if (indices[i] >= 0) {
          this->data[i].mac_Address = WiFi.BSSIDstr(indices[i]).replace(":", "");
          this->data[i].mac_RSSI = String(WiFi.RSSI(indices[i]));
          this->data[i].mac_Channel = String(WiFi.channel(indices[i]));
          this->data[i].mac_SSID = String(WiFi.SSID(indices[i]));
        }
      }
      mtx.unlock();
    }
  }
  macInfoField* getWifiData() {
    mtx.lock();
    macInfoField* a = this->data;
    this->size = this->data_size;
    mtx.unlock();
    return a;
  }
  uint8_t size = 0;
private:
  WiFiClass * w = NULL;
  oledclass* display;
  macInfoField *data;
  std::mutex mtx;
  uint8_t data_size = 0;
};

//  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
//    //Serial.println("Connection Failed! Rebooting...");
//    display.drawString (0, 20, "WiFi connection Failed! Rebooting...");
//    display.display ();
//    delay(5000);
//    ESP.restart();
//  }