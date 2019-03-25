#include <mutex>
#include <pthread.h>

/// <summary>
/// Class that parse GPS data from serial, needs the <typeparamref name="pin_tx"/> and <typeparamref name="pin_rx"/> as a number.
/// <typeparamref name="pin_gpsmodule_enable"/> must be a number, can be zero to disable the functionality.
/// <typeparamref name="debug"/> must be true for print all serial communication, false for disable debug.
/// </summary>
/// <typeparam name="pin_tx">Pin number for serial transmitting to gps module</typeparam>
/// <typeparam name="pin_rx">Pin number for serial receiving from gps module</typeparam>
/// <typeparam name="pin_gpsmodule_enable">Pin number or zero to disable</typeparam>
/// <typeparam name="debug">Print out serial gps communication</typeparam>
template <int pin_tx, int pin_rx, int pin_gpsmodule_enable, bool debug>
class GPS {
  public:
    /// <summary>Mutex variable to interrupt gps receiving thread</summary>
    pthread_mutex_t MutexGps;

    /// <summary>Constructor for GPS Parsing class, setup the io pins</summary>
    /// <typeparam name="wlanclass">Needs an instance of wlanclass for debug output</typeparam>
    GPS(wlanclass* wlanclass) {
      this->wlan = wlanclass;
      this->hs = new HardwareSerial(1);
      this->SetupIO();
    }

    #pragma region Start Stop
    /// <summary>Begin of GPS functionality, setups serial communication</summary>
    void Begin() {
      this->wlan->Box("Gps Setup!", 60);
      this->hs->begin(9600, SERIAL_8N1, pin_rx, pin_tx);
      pthread_mutex_init(&this->MutexGps, NULL);
      this->wlan->Box("Gps Successfull", 70);
    }

    /// <summary>Stops the gps thread</summary>
    void Stop() {
      this->running = false;
    }
    #pragma endregion

    /// <summary>Thread that listen to gps over serial and parse the gps data</summary>
    void Measure() {
      this->ActivateDevice();
      while (this->running) {
        pthread_mutex_lock(&this->MutexGps);
        char c = this->hs->read();
        this->Parse(c);
        if (debug) {
          if (c != 255) {
            Serial.print(c);
          }
          while(Serial.available()) {
            this->hs->write(Serial.read());
          }
        }
        pthread_mutex_unlock(&this->MutexGps);
        delay(1);
      }
      this->DeactivateDevice();
    }

    /// <summary>Indicates if there was received a full set of gps data.</summary>
    /// <returns>Returns true if full sentence arrived, otherwise false.</returns>
    bool HasData() {
      return this->hasData;
    }

    /// <summary>Returns parsed gps data</summary>
    /// <returns>struct gpsInfoField, with all nessesary gps informations</returns>
    gpsInfoField GetGPSData() {
      mtx.lock();
      gpsInfoField g = this->gpsdata;
      mtx.unlock();
      return g;
    }

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
    String data_rmc;
    bool running = true;
    bool hasData = false;

    void SetupIO() {
      if(pin_gpsmodule_enable != 0) {
        pinMode(pin_gpsmodule_enable, OUTPUT);
      }
    }

    void ActivateDevice() {
      if(pin_gpsmodule_enable != 0) {
        digitalWrite(pin_gpsmodule_enable, LOW);
      }
    }

    void DeactivateDevice() {
      if(pin_gpsmodule_enable != 0) {
        digitalWrite(pin_gpsmodule_enable, HIGH);
      }
    }

    #pragma region Parsing
    void Parse(uint8_t c) {
      if(c == '\n') {
        if(this->data.startsWith("$GPGGA") || this->data.startsWith("$GNGGA")) {
          this->data_gga = this->data;
        } else if(this->data.startsWith("$GPGSA") || this->data.startsWith("$GNGSA")) {
          this->data_gsa = this->data;
        } else if(this->data.startsWith("$GPVTG") || this->data.startsWith("$GNVTG")) {
          this->data_vtg = this->data;
        }  else if(this->data.startsWith("$GNZDA")) {
          this->data_zda = this->data;
        } else if(this->data.startsWith("$GPRMC") || this->data.startsWith("$GNRMC")) {
          this->data_rmc = this->data;
        } else if(this->data.startsWith("$GPTXT")) {
          this->ParseGGA(this->data_gga);
          this->ParseGSA(this->data_gsa);
          this->ParseVTG(this->data_vtg);
          this->ParseZDA(this->data_zda);
          if(!this->gpsdata.fix) {
            this->gpsdata.latitude = 0;
            this->gpsdata.longitude = 0;
          }
          if(!this->data_gga.equals("") && !this->data_gsa.equals("") && !this->data_vtg.equals("") && !this->data_zda.equals("")) {
            this->hasData = true;
          }
        } else if(this->data.startsWith("$GPGLL") || this->data.startsWith("$GNGLL")) {
          this->ParseGGA(this->data_gga);
          this->ParseGSA(this->data_gsa);
          this->ParseVTG(this->data_vtg);
          this->ParseRMC(this->data_rmc);
          if(!this->gpsdata.fix) {
            this->gpsdata.latitude = 0;
            this->gpsdata.longitude = 0;
          }
          if(!this->data_gga.equals("") && !this->data_gsa.equals("") && !this->data_vtg.equals("") && !this->data_rmc.equals("")) {
            this->hasData = true;
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

    void ParseGGA(String data) {
      if(data.indexOf(',') != -1) { //GGA          Global Positioning System Fix Data
        data = data.substring(data.indexOf(',')+1);
      }
      if(data.indexOf(',') != -1) { //172135.000   Fix taken at 12:35:19 UTC
        this->ParseTime(data.substring(0, data.indexOf(',')));
        data = data.substring(data.indexOf(',') + 1);
      }
      String lat;
      if(data.indexOf(',') != -1) { //5047.0438,N  Latitude 48 deg 07.038' N
        lat = data.substring(0, data.indexOf(','));
        data = data.substring(data.indexOf(',') + 1);
      }
      if(data.indexOf(',') != -1 && lat.length() != 0) { //5047.0438,N  Latitude 48 deg 07.038' N
        this->ParseCoord(lat, data.substring(0, data.indexOf(',')), 'A');
        data = data.substring(data.indexOf(',') + 1);
      }
      String lon;
      if(data.indexOf(',') != -1) { //00713.7223,E Longitude 11 deg 31.000' E
        lon = data.substring(0, data.indexOf(','));
        data = data.substring(data.indexOf(',') + 1);
      }
      if(data.indexOf(',') != -1 && lon.length() != 0) { //00713.7223,E Longitude 11 deg 31.000' E
        this->ParseCoord(lon, data.substring(0, data.indexOf(',')), 'O');
        data = data.substring(data.indexOf(',') + 1);
      }
      if(data.indexOf(',') != -1) { //1            Fix quality: 0 = invalid, 1 = GPS fix (SPS), 2 = DGPS fix, 3 = PPS fix, 4 = Real Time Kinematic, 5 = Float RTK, 6 = estimated (dead reckoning) (2.3 feature), 7 = Manual input mode, 8 = Simulation mode
        this->ParseFix(data.substring(0, data.indexOf(',')));
        data = data.substring(data.indexOf(',') + 1);
      }
      if(data.indexOf(',') != -1) { //08           Number of satellites being tracked
        this->ParseSatelites(data.substring(0, data.indexOf(',')));
        data = data.substring(data.indexOf(',') + 1);
      }
      if(data.indexOf(',') != -1) { //1.2          Horizontal dilution of position
        this->ParseHdop(data.substring(0, data.indexOf(',')));
        data = data.substring(data.indexOf(',') + 1);
      }
      if(data.indexOf(',') != -1) { //116.2,M      Altitude, Meters, above mean sea level
        this->ParseHeight(data.substring(0, data.indexOf(',')));
      }
    }

    void ParseGSA(String data) {
      if(data.indexOf(',') != -1) { //GSA      Satellite status
        data = data.substring(data.indexOf(',') + 1);
      }
      if(data.indexOf(',') != -1) { //A        Auto selection of 2D or 3D fix (M = manual)
        data = data.substring(data.indexOf(',') + 1);
      }
      if(data.indexOf(',') != -1) { //3        3D fix - values include: 1 = no fix, 2 = 2D fix, 3 = 3D fix
        this->ParseFixtype(data.substring(0, data.indexOf(',')));
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
        this->ParsePdop(data.substring(0, data.indexOf(',')));
        data = data.substring(data.indexOf(',') + 1);
      }
      if(data.indexOf(',') != -1) { //1.2      Horizontal dilution of precision (HDOP) 
        data = data.substring(data.indexOf(',') + 1);
      }
      if(data.indexOf('*') != -1) { //1.8      Vertical dilution of precision (VDOP)
        this->ParseVdop(data.substring(0, data.indexOf('*')));
      }
    }

    void ParseVTG(String data) {
      if(data.indexOf(',') != -1) { //VTG          Track made good and ground speed
        data = data.substring(data.indexOf(',') + 1);
      }
      if(data.indexOf(',') != -1) { //0.00,T       True track made good (degrees)
        this->ParseDir(data.substring(0, data.indexOf(',')));
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
        this->ParseSpeed(data.substring(0, data.indexOf(',')));
      }
    }

    void ParseZDA(String data) {
      if(data.indexOf(',') != -1) { //ZDA
        data = data.substring(data.indexOf(',') + 1);
      }
      if(data.indexOf(',') != -1) { //172135.000 HrMinSec(UTC)
        data = data.substring(data.indexOf(',') + 1);
      }
      if(data.indexOf(',') != -1) { //17,09,2018 Day,Month,Year
        this->ParseDay(data.substring(0, data.indexOf(',')));
        data = data.substring(data.indexOf(',') + 1);
      }
      if(data.indexOf(',') != -1) { //17,09,2018 Day,Month,Year
        this->ParseMonth(data.substring(0, data.indexOf(',')));
        data = data.substring(data.indexOf(',') + 1);
      }
      if(data.indexOf(',') != -1) { //17,09,2018 Day,Month,Year
        this->ParseYear(data.substring(0, data.indexOf(',')));
      }
    }

    void ParseRMC(String data) {
      if(data.indexOf(',') != -1) { //RMC          Recommended Minimum sentence C
        data = data.substring(data.indexOf(',') + 1);
      }
      if(data.indexOf(',') != -1) { //123519       Fix taken at 12:35:19 UTC
        data = data.substring(data.indexOf(',') + 1);
      }
      if(data.indexOf(',') != -1) { //A            Status A=active or V=Void.
        data = data.substring(data.indexOf(',') + 1);
      }
      if(data.indexOf(',') != -1) { //4807.038,N   Latitude 48 deg 07.038' N
        data = data.substring(data.indexOf(',') + 1);
      }
      if(data.indexOf(',') != -1) { //4807.038,N   Latitude 48 deg 07.038' N
        data = data.substring(data.indexOf(',') + 1);
      }
      if(data.indexOf(',') != -1) { //01131.000,E  Longitude 11 deg 31.000' E
        data = data.substring(data.indexOf(',') + 1);
      }
      if(data.indexOf(',') != -1) { //01131.000,E  Longitude 11 deg 31.000' E
        data = data.substring(data.indexOf(',') + 1);
      }
      if(data.indexOf(',') != -1) { //022.4        Speed over the ground in knots
        data = data.substring(data.indexOf(',') + 1);
      }
      if(data.indexOf(',') != -1) { //084.4        Track angle in degrees True
        data = data.substring(data.indexOf(',') + 1);
      }
      if(data.indexOf(',') != -1) { //230394       Date - 23rd of March 1994
        this->ParseDate(data.substring(0, data.indexOf(',')));
      }
    }

    void ParseTime(String t) {
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

    void ParseFix(String t) {
      if(t.length() == 1) {
        mtx.lock();
        this->gpsdata.fix = !t.equals("0");
        mtx.unlock();
      }
    }

    void ParseSatelites(String t) {
      if(t.length() >= 1) {
        mtx.lock();
        this->gpsdata.Satellites = t.toInt();
        mtx.unlock();
      }
    }

    void ParseHdop(String t) {
      if(t.length() >= 2 && t.indexOf('.') != -1) {
        mtx.lock();
        this->gpsdata.hdop = t.toFloat();
        mtx.unlock();
      }
    }

    void ParseVdop(String t) {
      if(t.length() >= 2 && t.indexOf('.') != -1) {
        mtx.lock();
        this->gpsdata.vdop = t.toFloat();
        mtx.unlock();
      }
    }

    void ParsePdop(String t) {
      if(t.length() >= 2 && t.indexOf('.') != -1) {
        mtx.lock();
        this->gpsdata.pdop = t.toFloat();
        mtx.unlock();
      }
    }

    void ParseHeight(String t) {
      if(t.length() >= 2 && t.indexOf('.') != -1) {
        mtx.lock();
        this->gpsdata.height = t.toFloat();
        mtx.unlock();
      }
    }

    void ParseCoord(String c, String d, uint8_t t) {
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

    void ParseFixtype(String t) {
      if(t.length() == 1) {
        mtx.lock();
        this->gpsdata.fixtype = t.toInt();
        mtx.unlock();
      }
    }

    void ParseDir(String t) {
      if(t.length() >= 2 && t.indexOf('.') != -1) {
        mtx.lock();
        this->gpsdata.direction = t.toFloat();
        mtx.unlock();
      }
    }

    void ParseSpeed(String t) {
      if(t.length() >= 2 && t.indexOf('.') != -1) {
        mtx.lock();
        this->gpsdata.speed = t.toFloat();
        mtx.unlock();
      }
    }

    void ParseDay(String t) {
      if(t.length() >= 1) {
        mtx.lock();
        this->gpsdata.day = t.toInt();
        mtx.unlock();
      }
    }

    void ParseMonth(String t) {
      if(t.length() >= 1) {
        mtx.lock();
        this->gpsdata.month = t.toInt();
        mtx.unlock();
      }
    }

    void ParseYear(String t) {
      if(t.length() >= 1) {
        mtx.lock();
        this->gpsdata.year = t.toInt();
        mtx.unlock();
      }
    }

    void ParseDate(String t) {
      if(t.length() == 6) {
        mtx.lock();
        this->gpsdata.day = t.substring(0, 2).toInt();
        this->gpsdata.month = t.substring(2, 4).toInt();
        this->gpsdata.year = t.substring(4, 6).toInt() + 2000;
        mtx.unlock();
      }
    }
    #pragma endregion
};
