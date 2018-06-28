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
  String mac_Address; // 100 character array
  String mac_RSSI; // 100 character array
  String mac_Channel;
  String mac_SSID;
};