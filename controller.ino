#include <Adafruit_NeoPixel.h>
#include <RotaryEncoder.h>
#include <Adafruit_SH110X.h>
#include <Keyboard.h>
#include "marco.h"

using namespace marco;

void checkPosition(RotaryEncoder* re) {
  re->tick();
}

// DisplayConfiguration implementation
DisplayConfiguration::DisplayConfiguration(String header) {
    headerText = header;
}

Controller::Controller(Adafruit_NeoPixel* npx, Adafruit_SH1106G* ash, RotaryEncoder* re, DisplayConfiguration* dconf) {
  pixels = npx;
  display = ash;
  encoder = re;
  dc = dconf;

  playStartupTone();
  // Create the neopixel strip with the built in definitions NUM_NEOPIXEL and PIN_NEOPIXEL
  // start pixels!
  pixels->begin();
  pixels->setBrightness(255);
  pixels->show(); // Initialize all pixels to 'off'

  setupDisplay();

  Keyboard.begin();
  
  encoderPos = encoder->getPosition();

  // set all mechanical keys to inputs
  for (uint8_t i=0; i<=12; i++) {
    pinMode(i, INPUT_PULLUP);
  }

  // We will use I2C for scanning the Stemma QT port
  Wire.begin();
  iteration = 0;
}
void Controller::refresh() { 
  // Scanning takes a while so we don't do it all the time
  if ((iteration & 0x3F) == 0) {
    scanI2c();
  }

  refreshDisplay();

  encoder->tick();          // check the encoder
  encoderPos = encoder->getPosition();

  for(int i=0; i< pixels->numPixels(); i++) {
    pixels->setPixelColor(i, Wheel(((i * 256 / pixels->numPixels()) + iteration) & 255));
  }

  for (int i=1; i<=12; i++) {
    if (!digitalRead(i) && !pressed[i]) { // switch pressed!
      pressed[i] = true;
      Serial.print("Switch "); Serial.println(i);
      pixels->setPixelColor(i-1, 0xFFFFFF);  // make white
      // move the text into a 3x4 grid
      display->setCursor(((i-1) % 3)*48, 32 + ((i-1)/3)*8);
      display->print("KEY");
      display->print(i);
      char strokes[] = {KEY_DELETE};
      size_t elems = 1;
      sendKeyCombo(strokes, elems);
    } else if (digitalRead(i)) {
      pressed[i] = false;
    }
  }

  // show neopixels, increment swirl
  pixels->show();
  iteration++;
}
void Controller::setupDisplay() {
  // Start OLED
  display->begin(0, true); // we dont use the i2c address but we will reset!
  display->display();

  // text display tests
  display->setTextSize(1);
  display->setTextWrap(false);
  display->setTextColor(SH110X_WHITE, SH110X_BLACK); // white text, black background
}
void Controller::refreshDisplay() {
  display->clearDisplay();
  display->setCursor(0,0);
  display->println(dc->headerText);

  // check encoder press
  display->setCursor(0, 24);
  if (!digitalRead(PIN_SWITCH)) {
    Serial.println("Encoder button");
    display->print("Encoder pressed ");
    pixels->sright!
  } else {
    pixels->setBrightness(80);
  }

  display->setCursor(0, 8);
  display->print("Rotary encoder: ");
  display->print(encoderPos);

  // display oled
  display->display();
}
void Controller::sendKeyCombo(char keys[], size_t elems) {
  for (int i = 0; i < elems; i++) {
    Keyboard.press(keys[i]);
    delay(100);
  }
  Keyboard.releaseAll();
}
void Controller::playStartupTone() {
  // Enable speaker
  pinMode(PIN_SPEAKER_ENABLE, OUTPUT);
  digitalWrite(PIN_SPEAKER_ENABLE, HIGH);
  // Play some tones
  pinMode(PIN_SPEAKER, OUTPUT);
  digitalWrite(PIN_SPEAKER, LOW);
  tone(PIN_SPEAKER, 988, 100);  // tone1 - B5
  delay(100);
  tone(PIN_SPEAKER, 1319, 200); // tone2 - E6
  delay(200);
}
void Controller::scanI2c() {
  Serial.println("Scanning I2C: ");
  Serial.print("Found I2C address 0x");
  for (uint8_t address = 0; address <= 0x7F; address++) {
    Wire.beginTransmission(address);
    i2c_found[address] = (Wire.endTransmission () == 0);
    if (i2c_found[address]) {
      Serial.print("0x");
      Serial.print(address, HEX);
      Serial.print(", ");
    }
  }
  Serial.println();
}
// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Controller::Wheel(byte WheelPos) {
  if(WheelPos < 85) {
  return pixels->Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else if(WheelPos < 170) {
  WheelPos -= 85;
  return pixels->Color(0, WheelPos * 3, 255 - WheelPos * 3);
  } else {
  WheelPos -= 170;
  return pixels->Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
}