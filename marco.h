#include <Adafruit_SH110X.h>

#ifndef _MARCO_H
#define _MARCO_H

namespace marco {
  class DisplayConfiguration {
    public:
      bool pixels[64][128];
      String headerText;
      DisplayConfiguration(String header);
  };
  class KeypressHandler {
  public:
    virtual void handle();
  };
  class Key {
    public:
      uint8_t index;
      KeypressHandler *handler;
      uint32_t color;
      bool pressed;
      void press();
      Key(uint8_t index, KeypressHandler *h);
  };
  class Controller {
    // Hardware components
    Key* keys[12];
    Adafruit_NeoPixel *pixels; // key LEDs
    Adafruit_SH1106G *display;
    DisplayConfiguration *dc;
    RotaryEncoder *encoder;
    // Trackers
    int encoderPos;
    bool i2c_found[128];
    bool pressed[12];
    String headerText;
    uint8_t iteration;
    public:
      Controller(Adafruit_NeoPixel* npx, Adafruit_SH1106G* ash, RotaryEncoder* re, DisplayConfiguration* dconf);
      ~Controller();
      void refresh();
    private:
      void setupDisplay();
      void refreshDisplay();
      void sendKeyCombo(char keys[], size_t elems);
      void playStartupTone();
      void scanI2c();
      uint32_t Wheel(byte WheelPos);
  };
}

#endif // _MARCO_H