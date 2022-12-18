#include <Adafruit_NeoPixel.h>
#include <RotaryEncoder.h>
#include <Wire.h>
#include <Keyboard.h>
#include "marco.h"

using namespace marco;

class Marco {
  public:
    Controller* controller;
    DisplayConfiguration* dc;
    Marco(Adafruit_NeoPixel* anp, Adafruit_SH1106G* ash, RotaryEncoder* re, DisplayConfiguration* dconf) {
      controller = new Controller(anp, ash, re, dconf);
      dc = dconf;
    }
    void refresh() {
      controller->refresh();
    }
};

Marco *mc;

Adafruit_NeoPixel* pixels;
Adafruit_SH1106G* display;
RotaryEncoder* ec;
DisplayConfiguration* dc;

void checkPosition() {
  ec->tick();
}

void setup() {
  pixels = new Adafruit_NeoPixel(NUM_NEOPIXEL, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);
  display = new Adafruit_SH1106G(128, 64, &SPI1, OLED_DC, OLED_RST, OLED_CS);
  ec = new RotaryEncoder(PIN_ROTA, PIN_ROTB, RotaryEncoder::LatchMode::FOUR3);
  dc = new DisplayConfiguration("marco");

  // set rotary encoder inputs and interrupts
  pinMode(PIN_ROTA, INPUT_PULLUP);
  pinMode(PIN_ROTB, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_ROTA), checkPosition, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_ROTB), checkPosition, CHANGE);  

  mc = new Marco(pixels, display, ec, dc);
}

void loop() {
  mc->refresh();
}
