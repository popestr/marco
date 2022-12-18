// TODO: breakout keyboard functionality
#include "marco.h"

using namespace marco;

class KeypressHandler {
  public:
    virtual void handle() = 0;
};

class Key {
  public:
    int index;
    KeypressHandler *handler;
    uint32_t color;
    bool pressed;
    void press() {
      handler->handle();
    }
    Key() {}
    Key(KeypressHandler *h) {
      handler = h;
    }
};
