#include <WiFi.h>
#include <WiFiServer.h>
#include <mutex>

template<const char* ssid, const char* psk_key, const char* espname, int server_clients, int server_port, bool debug>
class WLAN {
  public:
    WLAN(oledclass* disp) {
      this->oled = disp;
    }

    void begin() {
      this->box("Setup Wifi!", 20);
      this->w = new WiFiClass();
      this->w->mode(WIFI_STA);
      this->w->setHostname(espname);
      this->w->begin(ssid, psk_key);
      if (this->w->waitForConnectResult() != WL_CONNECTED) {
        this->box("Not connected to WiFi", 25);
        this->_wifi_connected = false;
      }
      else {
        this->box(String("Connected to wifi: ") + this->toString(this->w->localIP()), 25);
        this->_wifi_connected = true;
        this->server_connect();
      }
    }

    void lock() {
      this->mtx.lock();
    }

    void unlock() {
      this->mtx.unlock();
    }

    void server_clienthandle() {
      if(this->server_hasClient()) {
        uint8_t i;
        for(i = 0; i < server_clients; i++) {
          if(!this->serverClients[i] || !this->serverClients[i].connected()) {
            if(this->serverClients[i]) {
              this->serverClients[i].stop();
            }
            this->serverClients[i] = this->s->available();
            this->clients++;
            this->serverClients[i].print(String("Hello on the Telnet of ") + String(espname) + String("\r\n"));
            this->log(String("New Client: ") + String(i) + String("\n"));
            break;
          }
        }
        if(i == server_clients) {
          WiFiClient cl = this->s->available();
          cl.print(String("Hello on the Telnet of ") + String(espname) + String("\r\nYou will be kicked\r\n"));
          cl.stop();
          this->log("Connection rejected\n");
        }
      }
    }

    uint8_t getNumClients() {
      return this->clients;
    }

    void stop() {
      if(this->_server_connected) {
        this->s->stop();
        this->_server_connected = false;
      }
      this->w->mode(WIFI_OFF);
      this->_wifi_connected = false;
      btStop();
    }

    #pragma region Logger
    void log(String text) {
      if(debug) {
        Serial.print(text);
      }
      if(text.substring(text.length() - 1).equals("\n")) {
        text = text + String("\r");
      }
      for(uint8_t i = 0; i < server_clients; i++) {
        if(this->serverClients[i] && this->serverClients[i].connected()) {
          this->serverClients[i].print(text);
        }
      }
    }
    String toString(IPAddress a) {
      return String(a[0]) + "." + String(a[1]) + "." + String(a[2]) + "." + String(a[3]);
    }
    void log(const char text) {
      this->log(String(text));
    }
    void box(String text, uint8_t percent) {
      this->oled->box(text, percent);
      this->log(text + String("\n"));
    }
    void clear() {
      this->oled->clear();
    }
    void drawString(int16_t x, int16_t y, String text) {
      this->oled->drawString(x, y, text);
      this->log(text + String("\n"));
    }
    void display() {
      this->oled->display();
    }
    void gps(gpsInfoField gpsInfo, float battery) {
      this->oled->gps(gpsInfo, battery);
      this->log(String("################################################\n"));
      this->log(String("GNSS FIX: ") + String(gpsInfo.gnssFix) + String("\n"));
      this->log(String("Satellites: ") + String(gpsInfo.Satellites) + String("\n"));
      this->log(String("Lat: ") + String(gpsInfo.latitude, 6) + String("\n"));
      this->log(String("Long: ") + String(gpsInfo.longitude, 6) + String("\n"));
      this->log(String("HDOP: ") + String(gpsInfo.HDOP, 6) + String("\n"));
      this->log(String("Fix Time: ") + String(gpsInfo.hour < 10 ? "0" : "") + String(gpsInfo.hour) + String(gpsInfo.minute < 10 ? "0" : "") + String(gpsInfo.minute) + String(gpsInfo.second < 10 ? "0" : "") + String(gpsInfo.second) + String("\n"));
      this->log(String("Battery: ") + String(battery, 2) + String("\n"));
    }
    #pragma endregion

    /*const static uint8_t size = 3;

    void measure() {
      uint8_t n = this->w->scanNetworks();
      String bssid;
      if(n == 0) {
        this->mtx.lock();
        for(uint8_t i = 0; i < this->size; i++) {
          this->data[i].mac_Address = "000000000000";
          this->data[i].mac_RSSI = "0000";
          this->data[i].mac_Channel = "00";
          this->data[i].mac_SSID = "";
        }
        this->mtx.unlock();
      } else {
        //Serial.println(String("Networks found: ") + String(n));
        int8_t indices[n];
        for(int i = 0; i < n; i++) {
          indices[i] = i;
        }
        for(int i = 0; i < n; i++) {
          for(int j = i + 1; j < n; j++) {
            if(WiFi.RSSI(indices[j]) > WiFi.RSSI(indices[i])) {
              std::swap(indices[i], indices[j]);
            }
          }
        }
        for(int i = 0; i < n; i++) {
          bssid = WiFi.SSID(indices[i]);
          for(int j = i + 1; j < n; j++) {
            if(bssid == WiFi.SSID(indices[j])) {
              indices[j] = -1;
            }
          }
        }
        this->lock();
        this->networks = 0;
        if(n > this->size) {
          n = this->size;
        }
        for(uint8_t i = 0; i < this->size; i++) {
          if(i < n && indices[i] >= 0) {
            String bssid = WiFi.BSSIDstr(indices[i]);
            bssid.replace(":", "");
            this->data[i].mac_Address = bssid;
            this->data[i].mac_RSSI = String(WiFi.RSSI(indices[i]));
            this->data[i].mac_Channel = String(WiFi.channel(indices[i]));
            this->data[i].mac_SSID = String(WiFi.SSID(indices[i]));
            this->networks++;
          } else {
            this->data[i].mac_Address = "000000000000";
            this->data[i].mac_RSSI = "0000";
            this->data[i].mac_Channel = "00";
            this->data[i].mac_SSID = "";
          }
        }
        this->unlock();
      }
    }

    uint8_t getNetworks() {
      this->lock();
      uint8_t n = this->networks;
      this->unlock();
      return n;
    }

    macInfoField *data = new macInfoField[this->size];*/
  private:
    WiFiClass * w = NULL;
    bool _wifi_connected = false;
    WiFiServer* s = NULL;
    bool _server_connected = false;
    WiFiClient serverClients[server_clients];
    oledclass* oled;
    std::mutex mtx;
    uint8_t networks = 0;
    uint8_t clients = 0;

    bool server_connect() {
      if(this->_wifi_connected) {
        this->box(String("Open server on port: ") + String(server_port), 30);
        this->s = new WiFiServer(server_port);
        this->s->begin();
        this->s->setNoDelay(true);
        this->_server_connected = true;
      }
      return false;
    }

    bool server_hasClient() {
      if(this->_server_connected) {
        return this->s->hasClient();
      }
      return false;
    }
};