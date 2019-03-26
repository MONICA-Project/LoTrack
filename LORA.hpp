#include <LoRa.h>

/// <summary>
/// Class that sends data over LORA.
/// </summary>
/// <example>
/// https://www.semtech.com/products/wireless-rf/lora-transceivers/SX1272
/// https://www.semtech.com/uploads/documents/sx1272.pdf
/// The SX1272 offers three bandwidth options of 125 kHz, 250 kHz, and 500 kHz with spreading factors ranging from 6 to 12.
/// The SX1273 offers the same bandwidth options with spreading factors from 6 to 9.
/// https://www.semtech.com/products/wireless-rf/lora-transceivers/SX1276
/// https://www.semtech.com/uploads/documents/DS_SX1276-7-8-9_W_APP_V5.pdf
/// The SX1276 and SX1279 offer bandwidth options ranging from 7.8 kHz to 500 kHz with spreading factors ranging from 6 to 12, and covering all available frequency bands.
/// The SX1277 offers the same bandwidth and frequency band options with spreading factors from 6 to 9.
/// The SX1278 offers bandwidths and spreading factor options, but only covers the lower UHF bands.
/// </example> 
/// <typeparam name="pin_miso">Pin number of MISO pin on the controller</typeparam>
/// <typeparam name="pin_mosi">Pin number of MOSI pin on the controller</typeparam>
/// <typeparam name="pin_sck">Pin number of SCK pin on the controller</typeparam>
/// <typeparam name="pin_ss">Pin number of CS pin on the controller</typeparam>
/// <typeparam name="pin_rst">Pin number of the controller pin where the resetpin is attached</typeparam>
/// <typeparam name="pin_dio">Pin number of the controller pin where the data in 0 is attached</typeparam>
/// <typeparam name="band">Hz of the sending frequency</typeparam>
/// <typeparam name="espname">String of the node name</typeparam>
/// <typeparam name="lbt">(listen before talk) if true, this class will wait and listen to the LORA module before send, otherwise it will send directly</typeparam>
/// <typeparam name="binary">if true, the data will packed into a short binary message, otherwise it will send as long plain text.</typeparam>
template<int pin_miso, int pin_mosi, int pin_sck, int pin_ss, int pin_rst, int pin_dio, long band, const char* espname, bool lbt, bool binary>
class LORA {
  public:
    /// <summary>Constructor for LORA class, setup the io pins</summary>
    /// <typeparam name="wlanclass">Needs an instance of wlanclass for debug output</typeparam>
    /// <typeparam name="storage">Needs an instance of storage for reading frequency correction</typeparam>
    LORA(wlanclass * wlanclass, Storage * storage) {
      this->wlan = wlanclass;
      this->storage = storage;
      this->lora = new LoRaClass();
      SPI.begin (pin_sck, pin_miso, pin_mosi, pin_ss);
      this->lora->setPins (pin_ss, pin_rst, pin_dio);
    }

    /// <summary>Setup the LORA settings and start the module</summary>
    void Begin() {
      this->wlan->Box("Setup Lora!", 80);
      if (!this->lora->begin (band + this->storage->ReadOffsetFreq())) {
        this->wlan->Box("Lora Failed!", 90);
      } else {
        this->lora->setSignalBandwidth(125000);
        this->lora->setSpreadingFactor(10);
        this->lora->setCodingRate4(7);
        this->lora->setTxPower(20);
        this->lora->enableCrc();
        this->wlan->Box("Lora successful", 90);
        this->_lora_enabled = true;
      }
    }

    #pragma region Debug-Mode for Frequenztuning
    /// <summary>Enable Debugmode</summary>
    void Debugmode() {
      this->lora->setSignalBandwidth(1);
    }

    /// <summary>Set an offset to the center frequency</summary>
    /// <typeparam name="o">Frequency offset in Hz</typeparam>
    void SetFreqOffset(int32_t o) {
      this->lora->setFrequency(band + o);
    }

    /// <summary>Send TEST TEST TEST over lora</summary>
    void DebugSend() {
      this->lora->idle();
      this->lora->beginPacket();
      this->lora->print("TEST TEST TEST");
      this->lora->endPacket();
      this->lora->sleep();
    }
    #pragma endregion

    #pragma region Send Data
    /// <summary>Send a string over LORA</summary>
    /// <typeparam name="data">Text that should be send</typeparam>
    void Send(String data) {
      long startWait, endWait;
      if(lbt) {
        startWait = millis();
        while(this->lora->hasChannelActivity()) {
          delay(1);
        }
        endWait = millis();
      }
      this->lora->idle();
      this->lora->beginPacket();
      this->lora->print(data);
      this->lora->endPacket();
      this->lora->sleep();
      this->wlan->Log(String("################################################\n"));
      if(lbt) {
        this->wlan->Log(String("Waiting: ") + String(endWait-startWait) + String(" ms\n"));
      }
      this->wlan->Log(data + String("\n"));
    }

    /// <summary>Send a binary array over LORA</summary>
    /// <typeparam name="data">Byte array of the data</typeparam>
    /// <typeparam name="size">length of the array</typeparam>
    void Send(uint8_t* data, uint8_t size) {
      long startWait, endWait;
      if(lbt) {
        startWait = millis();
        while(this->lora->hasChannelActivity()) {
        delay(1);
        }
        endWait = millis();
      }
      this->lora->idle();
      this->lora->beginPacket();
      this->lora->write(data, size);
      this->lora->endPacket();
      this->lora->sleep();
      this->wlan->Log(String("################################################\n"));
      if(lbt) {
        this->wlan->Log(String("Waiting: ") + String(endWait - startWait) + String(" ms\n"));
      }

      String g;
      for(uint8_t i = 0; i < size; i++) {
        g = g + String(data[i], HEX) + String(" ");
      }
      this->wlan->Log(g + String("\n"));
    }

    /// <summary>Send gps and battery information over LORA</summary>
    /// <typeparam name="gps">struct gpsInfoField, with all nessesary gps informations</typeparam>
    /// <typeparam name="batt">voltage value of the battery</typeparam>
    /// <typeparam name="panic">optional, if true data will send as panic item</typeparam>
    void Send(gpsInfoField gps, float batt, bool panic = false) {
      if(binary) {
        //Data 1+2+4+4+1+2+3+3+1 = 21 Char
        uint8_t lora_data[21];
        if(panic) {
        lora_data[0] = 'p';
        } else {
        lora_data[0] = 'b';
        }
        for(uint8_t i = 0; i < 2; i++) {
        if(strlen(espname) > i) {
          lora_data[i + 1] = esp_name[i];
        } else {
          lora_data[i + 1] = 0;
        }
        }
        uint64_t lat = *(uint64_t*)&gps.latitude;  lora_data[3]  = (lat >> 0) & 0xFF; lora_data[4]  = (lat >> 8) & 0xFF; lora_data[5]  = (lat >> 16) & 0xFF; lora_data[6]  = (lat >> 24) & 0xFF;
        uint64_t lon = *(uint64_t*)&gps.longitude; lora_data[7] = (lon >> 0) & 0xFF; lora_data[8]  = (lon >> 8) & 0xFF; lora_data[9]  = (lon >> 16) & 0xFF; lora_data[10]  = (lon >> 24) & 0xFF; 
        if(gps.hdop >= 25.5) { lora_data[11] = 255; } else if(gps.hdop <= 25.5 && gps.hdop > 0){ lora_data[11] = (uint8_t)(gps.hdop * 10); } else { lora_data[11] = 0; }
        lora_data[12] = (uint8_t)((((uint16_t)(gps.height * 10)) >> 0) & 0xFF); lora_data[13] = (uint8_t)((((uint16_t)(gps.height * 10)) >> 8) & 0xFF);
        lora_data[14] = String(gps.time.substring(0, 2)).toInt(); lora_data[15] = String(gps.time.substring(2, 4)).toInt(); lora_data[16] = String(gps.time.substring(4, 6)).toInt();
        lora_data[17] = gps.day; lora_data[18] = gps.month; lora_data[19] = (uint8_t)(gps.year - 2000);
        lora_data[20] = (uint8_t)((batt * 100)-230);
        this->Send(lora_data, 21);
        if(panic) {
        this->lora->setSpreadingFactor(11);
        this->Send(lora_data, 21);
        this->lora->setSpreadingFactor(12);
        this->Send(lora_data, 21);
        this->lora->setSpreadingFactor(10);
        }
      } else {
        //Gps 9+9+7+5+4 = 34 Char
        this->Send(String(espname) + "\n" + String(gps.latitude, 6) + "," + String(gps.longitude, 6) + "," + String(gps.time) + "," + String(gps.hdop, 2) + "," + String(gps.height, 1) + "," + String(batt, 2));
      }
    }

    /// <summary>Send status information over LORA</summary>
    /// <typeparam name="version">Internal versionsnumber of the node.</typeparam>
    /// <typeparam name="ip">Ip address of the node, if connected to wifi</typeparam>
    /// <typeparam name="ssid">Ssid of the connected wifi</typeparam>
    /// <typeparam name="wififlag">true if wifi is currently connected</typeparam>
    /// <typeparam name="battery">voltage value of the battery</typeparam>
    /// <typeparam name="freqoffset">currently used frequency offset</typeparam>
    /// <typeparam name="runningStatus">0 = for Shutdown node now, 1 = normal Statup (wifi on, no sleeping), 2 = Powersafe mode</typeparam>
    void Send(uint8_t version, String ip, String ssid, bool wififlag, float battery, int32_t freqoffset, uint8_t runningStatus) {
      this->Send("deb\n" + String(espname) + "\n" + String(version) + "," + ip + "," + ssid + "," + (wififlag ? "t" : "f") + "," + String(battery, 2) + "," + String(freqoffset)+","+String(runningStatus));
    }
    #pragma endregion
  private:
    wlanclass * wlan;
    LoRaClass * lora;
    Storage * storage;
    bool _lora_enabled = false;
};
