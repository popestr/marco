// TODO: breakout keyboard functionality
#include "marco.h"

using namespace marco;

void Key::press() {
  handler->handle();
}
Key::Key(uint8_t idx, KeypressHandler *h) {
  index = idx;
  handler = h;
}

