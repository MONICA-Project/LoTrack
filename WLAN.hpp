#include <WiFi.h>
#include <WiFiServer.h>
#include <mutex>

template<const char* ssid, const char* psk_key, const char* espname, int server_clients, int server_port, bool debug>
class WLAN {
  public:
    WLAN(oledclass* disp) {
      this->oled = disp;
    }

    #pragma region Start and Stop
    void begin() {
      this->box("Setup Wifi!", 20);
      this->w = new WiFiClass();
      this->w->mode(WIFI_STA);
      this->w->setHostname(espname);
      this->w->begin(ssid, psk_key);
      if(this->w->waitForConnectResult() != WL_CONNECTED) {
        this->box("Not connected to WiFi", 25);
        this->_wifi_connected = false;
      } else {
        this->box(String("Connected to wifi: ") + this->toString(this->w->localIP()), 25);
        this->_wifi_connected = true;
        this->server_connect();
      }
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
    #pragma endregion

    #pragma region Telnet Server
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

    bool server_has_data() {
      for(uint8_t i = 0; i < server_clients; i++) {
        if(this->serverClients[i] && this->serverClients[i].connected()) {
          if(this->serverClients[i].available()) {
            while(this->serverClients[i].available()) {
              uint8_t c = this->serverClients[i].read();
              if(c == '\n') {
                this->last_data = this->serverClientsData[i];
                this->serverClientsData[i] = String();
                return true;
              } else {
                if(c != '\r') {
                  String cn = String(' ');
                  cn.setCharAt(0, c);
                  this->serverClientsData[i].concat(cn);
                }
              }
            }
          }
        }
      }
      return false;
    }

    uint8_t getNumClients() {
      return this->clients;
    }

    String get_last_string() {
      return this->last_data;
    }
    #pragma endregion

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
      this->log(String("FIX: ") + String(gpsInfo.fix) + String("\n"));
      this->log(String("FIX-Type: ") + String(gpsInfo.fixtype) + String("\n"));
      this->log(String("Satellites: ") + String(gpsInfo.Satellites) + String("\n"));
      this->log(String("Lat: ") + String(gpsInfo.latitude, 6) + String("\n"));
      this->log(String("Long: ") + String(gpsInfo.longitude, 6) + String("\n"));
      this->log(String("HDOP: ") + String(gpsInfo.hdop, 2) + String("\n"));
      this->log(String("VDOP: ") + String(gpsInfo.vdop, 2) + String("\n"));
      this->log(String("PDOP: ") + String(gpsInfo.pdop, 2) + String("\n"));
      this->log(String("Height: ") + String(gpsInfo.height, 1) + String("\n"));
      this->log(String("Direction: ") + String(gpsInfo.direction, 2) + String("\n"));
      this->log(String("Speed: ") + String(gpsInfo.speed, 2) + String("\n"));
      this->log(String("Fix Time: ") + gpsInfo.time + String("\n"));
      this->log(String("Fix Date: ") + String(gpsInfo.day) + "." + String(gpsInfo.month) + "." + String(gpsInfo.year) + String("\n"));
      this->log(String("Battery: ") + String(battery, 2) + String("\n"));
    }
    #pragma endregion

  private:
    WiFiClass * w = NULL;
    bool _wifi_connected = false;
    WiFiServer* s = NULL;
    bool _server_connected = false;
    WiFiClient serverClients[server_clients];
    oledclass* oled;
    uint8_t clients = 0;
    String serverClientsData[server_clients];
    String last_data;

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