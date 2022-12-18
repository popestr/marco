// TODO: breakout keyboard functionality
#include "marco.h"

using namespace marco;

void Key::press() {
  handler->handle();
}
Key::Key() {}
Key::Key(KeypressHandler *h) {
  handler = h;
}

