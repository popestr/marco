#include <Adafruit_NeoPixel.h>
#include <RotaryEncoder.h>
#include <Wire.h>
#include <Keyboard.h>
#include "marco.h"

using namespace marco;

class Marco {
  public:
    marco::Controller controller;
    marco::DisplayConfiguration dc;
    Marco() {
      dc = DisplayConfiguration("marco"); 
      controller = Controller(&dc);     
    }
    void refresh() {
      controller.refresh();
    }
};
// our encoder position state
int encoder_pos = 0;
Marco mc;

char ctrl = KEY_LEFT_CTRL;
char alt = KEY_LEFT_ALT;

void setup() {
  mc = Marco();
}

void loop() {
  mc.refresh();
}
