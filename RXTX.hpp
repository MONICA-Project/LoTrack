
class RXTX {
public:
  RXTX() {
    Serial.begin(115200);
    while (!Serial);
  }
};