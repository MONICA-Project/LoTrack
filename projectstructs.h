/// <summary>Stucture for exchange GPS informations</summary>
struct gpsInfoField {
  bool fix = false;
  uint8_t fixtype = 1;
  float latitude = 0;
  float longitude = 0;
  float height = 0;
  uint8_t Satellites = 0;
  float hdop = 0;
  float pdop = 0;
  float vdop = 0;
  float direction = 0;
  float speed = 0;
  String time = "000000";
  uint8_t day = 0;
  uint8_t month = 0;
  uint16_t year = 0;
};

/// <summary>Structure for exchange wirelss network informations</summary>
struct macInfoField {
  String mac_Address = "000000000000"; // 12 character array
  String mac_RSSI = "0000"; // 4 character array
  String mac_Channel = "00";
  String mac_SSID = "";
};