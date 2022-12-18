#include <Adafruit_SH110X.h>

#ifndef _MARCO_H
#define _MARCO_H

namespace marco {
  class DisplayConfiguration {
    public:
      bool pixels[64][128];
      String headerText;
      DisplayConfiguration();
      DisplayConfiguration(String header);
  };
  class KeypressHandler {
  public:
    virtual void handle();
  };
  class Key {
    public:
      int index;
      KeypressHandler *handler;
      uint32_t color;
      bool pressed;
      void press();
      Key();
      Key(KeypressHandler *h);
  };
  class Controller {
    // Hardware components
    Key keys[12];
    Adafruit_NeoPixel pixels; // key LEDs
    Adafruit_SH1106G display;
    DisplayConfiguration dc;
    RotaryEncoder encoder;
    // Trackers
    int encoderPos;
    bool i2c_found[128];
    bool pressed[12];
    String headerText;
    uint8_t iteration;
    public:
      Controller();
      Controller(DisplayConfiguration* dconf);
      void refresh();
    private:
      void setupDisplay();
      void refreshDisplay();
      void sendKeyCombo();
      void playStartupTone();
      void scanI2c();
      uint32_t Wheel(byte WheelPos);
  };
}

#endif // _MARCO_H