#include <Preferences.h>

class Storage {
  public:
    Storage() {
      this->preferences = new Preferences();
    }
    void begin() {
      this->preferences->begin("finetune", false);
    }
    int32_t readOffsetFreq() {
      return this->preferences->getLong("finetune");
    }
    void writeOffsetFreq(int32_t o) {
      this->preferences->putLong("finetune", o);
    }
  private:
    Preferences * preferences = NULL;
};