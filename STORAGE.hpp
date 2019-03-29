#include <Preferences.h>

class Storage {
  public:
    Storage() {
      this->preferences = new Preferences();
    }

    void Begin() {
      this->preferences->begin("finetune", false);
      if(this->preferences->getLong("finetune", -2147483648) == -2147483648) {
        this->preferences->putLong("finetune", 0);
      }
    }

    int32_t ReadOffsetFreq() {
      return this->preferences->getLong("finetune", 0);
    }

    void WriteOffsetFreq(int32_t o) {
      this->preferences->putLong("finetune", o);
    }
  private:
    Preferences * preferences = NULL;
};