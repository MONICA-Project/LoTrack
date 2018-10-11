#include <Preferences.h>

class Storage {
  public:
    Storage() {
      this->preferences = new Preferences();
    }
    void Begin() {
      this->preferences->begin("finetune", false);
    }
    int32_t ReadOffsetFreq() {
      return this->preferences->getLong("finetune");
    }
    void WriteOffsetFreq(int32_t o) {
      this->preferences->putLong("finetune", o);
    }
  private:
    Preferences * preferences = NULL;
};