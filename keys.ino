// TODO: breakout keyboard functionality

class KeypressHandler {
  public:
    virtual void handle() = 0;
}

class Key {
  public:
    int index;
    KeypressHandler handler;
    uint32_t color;
    bool pressed;
    void press() {
      handler.handle();
    }
    Key(KeypressHandler h) {
      handler = h;
    }
}
