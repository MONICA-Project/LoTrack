struct gpsInfoField {
  bool gnssFix = false;
  float latitude = 0.0;
  float longitude = 0.0;
  int Satellites = 0;
  float HDOP = 0.0;
  int hour = 0;
  int minute = 0;
  int second = 0;
};

struct macInfoField {
  String mac_Address = "000000000000"; // 12 character array
  String mac_RSSI = "0000"; // 4 character array
  String mac_Channel = "00";
  String mac_SSID = "";
};