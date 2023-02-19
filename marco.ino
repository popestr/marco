#include <Adafruit_NeoPixel.h>
#include <RotaryEncoder.h>
#include <Wire.h>
#include <Keyboard.h>
#include "marco.h"

using namespace marco;

Controller *mothership;
Adafruit_NeoPixel *pixels;
Adafruit_SH1106G *display;
RotaryEncoder *ec;
DisplayConfiguration *dc;

void checkEncoderPosition()
{
  ec->tick();
}

void setup()
{
  pixels = new Adafruit_NeoPixel(NUM_NEOPIXEL, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);
  display = new Adafruit_SH1106G(128, 64, &SPI1, OLED_DC, OLED_RST, OLED_CS);
  ec = new RotaryEncoder(PIN_ROTA, PIN_ROTB, RotaryEncoder::LatchMode::FOUR3);
  dc = new DisplayConfiguration("marco");
  mothership = new Controller(pixels, display, ec, dc);

  // set rotary encoder inputs and interrupts
  pinMode(PIN_ROTA, INPUT_PULLUP);
  pinMode(PIN_ROTB, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_ROTA), checkEncoderPosition, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_ROTB), checkEncoderPosition, CHANGE);
}

void loop()
{
  mothership->refresh();
}
