/// <summary>Class to create a serial communication</summary>
class RXTX {
public:
  /// <summary>Constructor for Serial. Set Baudrate and waits until serial is activated</summary>
  RXTX() {
    Serial.begin(115200);
    while (!Serial);
  }
};