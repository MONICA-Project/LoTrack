#include <mutex>
#include <pthread.h>

template <int serial_pin, int pin_tx, int pin_rx, bool debug>
class GPS {
public:
  GPS(wlanclass* wlanclass) {
    this->wlan = wlanclass;
    this->hs = new HardwareSerial(serial_pin);
  }
  void begin() {
    this->wlan->box("Gps Setup!", 60);
    this->hs->begin(9600, SERIAL_8N1, pin_rx, pin_tx);
    this->wlan->box("Gps Successfull", 70);
  }
  void measure() {
    while (true) {
      pthread_mutex_lock(&this->mgp);
      char c = this->hs->read();
      this->parse(c);
      if (debug) {
        if (c != 255) {
          Serial.print(c);
        }
        while(Serial.available()) {
          this->hs->write(Serial.read());
        }
      }
      pthread_mutex_unlock(&this->mgp);
      delay(1);
    }
  }
  gpsInfoField getGPSData() {
    mtx.lock();
    gpsInfoField g = this->gpsdata;
    mtx.unlock();
    return g;
  }
  pthread_mutex_t mgp;
private:
  HardwareSerial * hs;
  wlanclass * wlan;
  gpsInfoField gpsdata;
  std::mutex mtx;
  String data;
  String data_gga;
  String data_gsa;
  String data_vtg;
  String data_zda;

  void parse(uint8_t c) {
    if(c == '\n') {
      if(this->data.startsWith("$GNGGA")) {
        this->data_gga = this->data;
      } else if(this->data.startsWith("$GPGSA")) {
        this->data_gsa = this->data;
      } else if(this->data.startsWith("$GNVTG")) {
        this->data_vtg = this->data;
      } else if(this->data.startsWith("$GNZDA")) {
        this->data_zda = this->data;
      } else if(this->data.startsWith("$GPTXT")) {
        this->parseGGA(this->data_gga);
        this->parseGSA(this->data_gsa);
        this->parseVTG(this->data_vtg);
        this->parseZDA(this->data_zda);
        if(!this->gpsdata.fix) {
          this->gpsdata.latitude = 0;
          this->gpsdata.longitude = 0;
        }
      } 
      this->data = String();
    } else {
      if(c != '\r' && c != 0xFF) {
        String cn = String(' ');
        cn.setCharAt(0, c);
        this->data.concat(cn);
      }
    }
  }

  void parseGGA(String data) {
    if(data.indexOf(',') != -1) { //GGA          Global Positioning System Fix Data
      data = data.substring(data.indexOf(',')+1);
    }
    if(data.indexOf(',') != -1) { //172135.000   Fix taken at 12:35:19 UTC
      this->parseTime(data.substring(0, data.indexOf(',')));
      data = data.substring(data.indexOf(',') + 1);
    }
    String lat;
    if(data.indexOf(',') != -1) { //5047.0438,N  Latitude 48 deg 07.038' N
      lat = data.substring(0, data.indexOf(','));
      data = data.substring(data.indexOf(',') + 1);
    }
    if(data.indexOf(',') != -1 && lat.length() != 0) { //5047.0438,N  Latitude 48 deg 07.038' N
      this->parseCoord(lat, data.substring(0, data.indexOf(',')), 'A');
      data = data.substring(data.indexOf(',') + 1);
    }
    String lon;
    if(data.indexOf(',') != -1) { //00713.7223,E Longitude 11 deg 31.000' E
      lon = data.substring(0, data.indexOf(','));
      data = data.substring(data.indexOf(',') + 1);
    }
    if(data.indexOf(',') != -1 && lon.length() != 0) { //00713.7223,E Longitude 11 deg 31.000' E
      this->parseCoord(lon, data.substring(0, data.indexOf(',')), 'O');
      data = data.substring(data.indexOf(',') + 1);
    }
    if(data.indexOf(',') != -1) { //1            Fix quality: 0 = invalid, 1 = GPS fix (SPS), 2 = DGPS fix, 3 = PPS fix, 4 = Real Time Kinematic, 5 = Float RTK, 6 = estimated (dead reckoning) (2.3 feature), 7 = Manual input mode, 8 = Simulation mode
      this->parseFix(data.substring(0, data.indexOf(',')));
      data = data.substring(data.indexOf(',') + 1);
    }
    if(data.indexOf(',') != -1) { //08           Number of satellites being tracked
      this->parseSatelites(data.substring(0, data.indexOf(',')));
      data = data.substring(data.indexOf(',') + 1);
    }
    if(data.indexOf(',') != -1) { //1.2          Horizontal dilution of position
      this->parseHdop(data.substring(0, data.indexOf(',')));
      data = data.substring(data.indexOf(',') + 1);
    }
    if(data.indexOf(',') != -1) { //116.2,M      Altitude, Meters, above mean sea level
      this->parseHeight(data.substring(0, data.indexOf(',')));
    }
  }

  void parseGSA(String data) {
    if(data.indexOf(',') != -1) { //GSA      Satellite status
      data = data.substring(data.indexOf(',') + 1);
    }
    if(data.indexOf(',') != -1) { //A        Auto selection of 2D or 3D fix (M = manual)
      data = data.substring(data.indexOf(',') + 1);
    }
    if(data.indexOf(',') != -1) { //3        3D fix - values include: 1 = no fix, 2 = 2D fix, 3 = 3D fix
      this->parseFixtype(data.substring(0, data.indexOf(',')));
      data = data.substring(data.indexOf(',') + 1);
    }
    if(data.indexOf(',') != -1) { //04,05... PRNs of satellites used for fix (space for 12)
      data = data.substring(data.indexOf(',') + 1);
    }
    if(data.indexOf(',') != -1) { //04,05... PRNs of satellites used for fix (space for 12)
      data = data.substring(data.indexOf(',') + 1);
    }
    if(data.indexOf(',') != -1) { //04,05... PRNs of satellites used for fix (space for 12)
      data = data.substring(data.indexOf(',') + 1);
    }
    if(data.indexOf(',') != -1) { //04,05... PRNs of satellites used for fix (space for 12)
      data = data.substring(data.indexOf(',') + 1);
    }
    if(data.indexOf(',') != -1) { //04,05... PRNs of satellites used for fix (space for 12)
      data = data.substring(data.indexOf(',') + 1);
    }
    if(data.indexOf(',') != -1) { //04,05... PRNs of satellites used for fix (space for 12)
      data = data.substring(data.indexOf(',') + 1);
    }
    if(data.indexOf(',') != -1) { //04,05... PRNs of satellites used for fix (space for 12)
      data = data.substring(data.indexOf(',') + 1);
    }
    if(data.indexOf(',') != -1) { //04,05... PRNs of satellites used for fix (space for 12)
      data = data.substring(data.indexOf(',') + 1);
    }
    if(data.indexOf(',') != -1) { //04,05... PRNs of satellites used for fix (space for 12)
      data = data.substring(data.indexOf(',') + 1);
    }
    if(data.indexOf(',') != -1) { //04,05... PRNs of satellites used for fix (space for 12)
      data = data.substring(data.indexOf(',') + 1);
    }
    if(data.indexOf(',') != -1) { //04,05... PRNs of satellites used for fix (space for 12)
      data = data.substring(data.indexOf(',') + 1);
    }
    if(data.indexOf(',') != -1) { //04,05... PRNs of satellites used for fix (space for 12)
      data = data.substring(data.indexOf(',') + 1);
    }
    if(data.indexOf(',') != -1) { //2.2      PDOP (dilution of precision)
      this->parsePdop(data.substring(0, data.indexOf(',')));
      data = data.substring(data.indexOf(',') + 1);
    }
    if(data.indexOf(',') != -1) { //1.2      Horizontal dilution of precision (HDOP) 
      data = data.substring(data.indexOf(',') + 1);
    }
    if(data.indexOf('*') != -1) { //1.8      Vertical dilution of precision (VDOP)
      this->parseVdop(data.substring(0, data.indexOf('*')));
    }
  }

  void parseVTG(String data) {
    if(data.indexOf(',') != -1) { //VTG          Track made good and ground speed
      data = data.substring(data.indexOf(',') + 1);
    }
    if(data.indexOf(',') != -1) { //0.00,T       True track made good (degrees)
      this->parseDir(data.substring(0, data.indexOf(',')));
      data = data.substring(data.indexOf(',') + 1);
    }
    if(data.indexOf(',') != -1) { //0.00,T       True track made good (degrees)
      data = data.substring(data.indexOf(',') + 1);
    }
    if(data.indexOf(',') != -1) { //,M           Magnetic track made good
      data = data.substring(data.indexOf(',') + 1);
    }
    if(data.indexOf(',') != -1) { //,M           Magnetic track made good
      data = data.substring(data.indexOf(',') + 1);
    }
    if(data.indexOf(',') != -1) { //0.00,N       Ground speed, knots
      data = data.substring(data.indexOf(',') + 1);
    }
    if(data.indexOf(',') != -1) { //0.00,N       Ground speed, knots
      data = data.substring(data.indexOf(',') + 1);
    }
    if(data.indexOf(',') != -1) { //0.00,K       Ground speed, Kilometers per hour
      this->parseSpeed(data.substring(0, data.indexOf(',')));
    }
  }

  void parseZDA(String data) {
    if(data.indexOf(',') != -1) { //ZDA
      data = data.substring(data.indexOf(',') + 1);
    }
    if(data.indexOf(',') != -1) { //172135.000 HrMinSec(UTC)
      data = data.substring(data.indexOf(',') + 1);
    }
    if(data.indexOf(',') != -1) { //17,09,2018 Day,Month,Year
      this->parseDay(data.substring(0, data.indexOf(',')));
      data = data.substring(data.indexOf(',') + 1);
    }
    if(data.indexOf(',') != -1) { //17,09,2018 Day,Month,Year
      this->parseMonth(data.substring(0, data.indexOf(',')));
      data = data.substring(data.indexOf(',') + 1);
    }
    if(data.indexOf(',') != -1) { //17,09,2018 Day,Month,Year
      this->parseYear(data.substring(0, data.indexOf(',')));
    }
  }

  void parseTime(String t) {
    if(t.length() > 6) {
      mtx.lock();
      this->gpsdata.time = t.substring(0, 6);
      mtx.unlock();
    } else if(t.length() == 6) {
      mtx.lock();
      this->gpsdata.time = t;
      mtx.unlock();
    }
  }

  void parseFix(String t) {
    if(t.length() == 1) {
      mtx.lock();
      this->gpsdata.fix = !t.equals("0");
      mtx.unlock();
    }
  }

  void parseSatelites(String t) {
    if(t.length() >= 1) {
      mtx.lock();
      this->gpsdata.Satellites = t.toInt();
      mtx.unlock();
    }
  }

  void parseHdop(String t) {
    if(t.length() >= 2 && t.indexOf('.') != -1) {
      mtx.lock();
      this->gpsdata.hdop = t.toFloat();
      mtx.unlock();
    }
  }

  void parseVdop(String t) {
    if(t.length() >= 2 && t.indexOf('.') != -1) {
      mtx.lock();
      this->gpsdata.vdop = t.toFloat();
      mtx.unlock();
    }
  }

  void parsePdop(String t) {
    if(t.length() >= 2 && t.indexOf('.') != -1) {
      mtx.lock();
      this->gpsdata.pdop = t.toFloat();
      mtx.unlock();
    }
  }

  void parseHeight(String t) {
    if(t.length() >= 2 && t.indexOf('.') != -1) {
      mtx.lock();
      this->gpsdata.height = t.toFloat();
      mtx.unlock();
    }
  }

  void parseCoord(String c, String d, uint8_t t) {
    if(t == 'A') {
      float coord = c.substring(0, 2).toFloat();
      coord = coord + (c.substring(2).toFloat() / 60);
      if(d.equals("S")) {
        coord = coord * -1;
      }
      mtx.lock();
      this->gpsdata.latitude = coord;
      mtx.unlock();
    } else if(t == 'O') {
      float coord = c.substring(0, 3).toFloat();
      coord = coord + (c.substring(3).toFloat() / 60);
      if(d.equals("W")) {
        coord = coord * -1;
      }
      mtx.lock();
      this->gpsdata.longitude = coord;
      mtx.unlock();
    }
  }

  void parseFixtype(String t) {
    if(t.length() == 1) {
      mtx.lock();
      this->gpsdata.fixtype = t.toInt();
      mtx.unlock();
    }
  }

  void parseDir(String t) {
    if(t.length() >= 2 && t.indexOf('.') != -1) {
      mtx.lock();
      this->gpsdata.direction = t.toFloat();
      mtx.unlock();
    }
  }

  void parseSpeed(String t) {
    if(t.length() >= 2 && t.indexOf('.') != -1) {
      mtx.lock();
      this->gpsdata.speed = t.toFloat();
      mtx.unlock();
    }
  }

  void parseDay(String t) {
    if(t.length() >= 1) {
      mtx.lock();
      this->gpsdata.day = t.toInt();
      mtx.unlock();
    }
  }

  void parseMonth(String t) {
    if(t.length() >= 1) {
      mtx.lock();
      this->gpsdata.month = t.toInt();
      mtx.unlock();
    }
  }

  void parseYear(String t) {
    if(t.length() >= 1) {
      mtx.lock();
      this->gpsdata.year = t.toInt();
      mtx.unlock();
    }
  }
};
