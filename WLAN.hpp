#include <WiFi.h>
#include <WiFiServer.h>
#include <mutex>

template<const char* ssid, const char* psk_key, int server_clients, int server_port, bool debug>
class WLAN {
  public:
    WLAN(oledclass* disp, Storage * storage) {
      this->oled = disp;
      this->storage = storage;
    }

    #pragma region Start and Stop
    void Begin() {
      this->w = new WiFiClass();
    }

    void Connect() {
      this->Box("Setup Wifi!", 20);
      this->w->mode(WIFI_STA);
      this->w->setHostname(this->storage->GetEspname().c_str());
      this->Log(String("MAC: ") + this->w->macAddress() + String("\n"));
      this->w->begin(ssid, psk_key);
      if(this->w->waitForConnectResult() != WL_CONNECTED) {
        this->Box("Not connected to WiFi", 25);
        this->_wifi_connected = false;
      } else {
        this->Box(String("Connected to wifi: ") + this->GetIp(), 25);
        this->_wifi_connected = true;
        this->ServerConnect();
      }
    }

    void Stop() {
      if(this->_server_connected) {
        this->s->stop();
        this->_server_connected = false;
      }
      this->w->mode(WIFI_OFF);
      this->_wifi_connected = false;
      btStop();
    }
    #pragma endregion

    #pragma region Public Attributes
    String GetIp() {
      return this->ToString(this->w->localIP());
    }

    String GetSsid() {
      return String(ssid);
    }

    bool GetStatus() {
      return this->_wifi_connected;
    }
    #pragma endregion

    #pragma region Telnet Server
    void ServerClienthandle() {
      if(this->ServerHasClient()) {
        uint8_t i;
        for(i = 0; i < server_clients; i++) {
          if(!this->serverClients[i] || !this->serverClients[i].connected()) {
            if(this->serverClients[i]) {
              this->serverClients[i].stop();
            }
            this->serverClients[i] = this->s->available();
            this->clients++;
            this->serverClients[i].print(String("Hello on the Telnet of ") + this->storage->GetEspname() + String("\r\n"));
            this->Log(String("New Client: ") + String(i) + String("\n"));
            break;
          }
        }
        if(i == server_clients) {
          WiFiClient cl = this->s->available();
          cl.print(String("Hello on the Telnet of ") + this->storage->GetEspname() + String("\r\nYou will be kicked\r\n"));
          cl.stop();
          this->Log("Connection rejected\n");
        }
      }
    }

    bool ServerHasData() {
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

    uint8_t GetNumClients() {
      return this->clients;
    }

    String GetLastString() {
      return this->last_data;
    }
    #pragma endregion

    #pragma region Logger
    void Log(String text) {
      if(debug) {
        Serial.print(text);
      }
      text.replace("\n","\n\r");
      for(uint8_t i = 0; i < server_clients; i++) {
        if(this->serverClients[i] && this->serverClients[i].connected()) {
          this->serverClients[i].print(text);
        }
      }
    }

    String ToString(IPAddress a) {
      return String(a[0]) + "." + String(a[1]) + "." + String(a[2]) + "." + String(a[3]);
    }

    void Log(const char text) {
      this->Log(String(text));
    }

    void Box(String text, uint8_t percent) {
      this->oled->box(text, percent);
      this->Log(text + String("\n"));
    }

    void Clear() {
      this->oled->clear();
    }

    void DrawString(int16_t x, int16_t y, String text) {
      this->oled->drawString(x, y, text);
      this->Log(text + String("\n"));
    }

    void Display() {
      this->oled->display();
    }

    void Gps(gpsInfoField gpsInfo, float battery) {
      this->oled->gps(gpsInfo, battery);
      this->Log(String("################################################\n"));
      this->Log(String("FIX: ") + String(gpsInfo.fix) + String("\n"));
      this->Log(String("FIX-Type: ") + String(gpsInfo.fixtype) + String("\n"));
      this->Log(String("Satellites: ") + String(gpsInfo.Satellites) + String("\n"));
      this->Log(String("Lat: ") + String(gpsInfo.latitude, 6) + String("\n"));
      this->Log(String("Long: ") + String(gpsInfo.longitude, 6) + String("\n"));
      this->Log(String("HDOP: ") + String(gpsInfo.hdop, 2) + String("\n"));
      this->Log(String("VDOP: ") + String(gpsInfo.vdop, 2) + String("\n"));
      this->Log(String("PDOP: ") + String(gpsInfo.pdop, 2) + String("\n"));
      this->Log(String("Height: ") + String(gpsInfo.height, 1) + String("\n"));
      this->Log(String("Direction: ") + String(gpsInfo.direction, 2) + String("\n"));
      this->Log(String("Speed: ") + String(gpsInfo.speed, 2) + String("\n"));
      this->Log(String("Fix Time: ") + gpsInfo.time + String("\n"));
      this->Log(String("Fix Date: ") + String(gpsInfo.day) + "." + String(gpsInfo.month) + "." + String(gpsInfo.year) + String("\n"));
      this->Log(String("Battery: ") + String(battery, 2) + String("\n"));
    }
    #pragma endregion

  private:
    WiFiClass * w = NULL;
    bool _wifi_connected = false;
    WiFiServer* s = NULL;
    bool _server_connected = false;
    WiFiClient serverClients[server_clients];
    oledclass* oled;
    Storage * storage;
    uint8_t clients = 0;
    String serverClientsData[server_clients];
    String last_data;

    bool ServerConnect() {
      if(this->_wifi_connected) {
        this->Box(String("Open server on port: ") + String(server_port), 30);
        this->s = new WiFiServer(server_port);
        this->s->begin();
        this->s->setNoDelay(true);
        this->_server_connected = true;
      }
      return false;
    }

    bool ServerHasClient() {
      if(this->_server_connected) {
        return this->s->hasClient();
      }
      return false;
    }
};