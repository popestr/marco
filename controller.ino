#include <Adafruit_NeoPixel.h>
#include <RotaryEncoder.h>
#include <Adafruit_SH110X.h>
#include <Keyboard.h>
#include <Mouse.h>
#include "marco.h"
#include <string>

#define PROG_CLIPBOARD        0x0
#define PRIME                 0x0001
#define CANCEL_PRIME          0x0002
#define REQUEST_CLIP          0x0003

#define ARD_KEY_LED           0xF
#define SET_COLOR             0x0001
#define TURN_OFF              0x0002

const unsigned int MAX_MESSAGE_LENGTH = 35;
const unsigned int NUM_KEYS = 12;

using namespace marco;

class Clipboard : public Key {
  public:
    bool hasClip = false;
    bool waitingForClip = false;

    void press() {
      Instruction instruction(PROG_CLIPBOARD, index);
      if(!hasClip) {
        waitingForClip = !waitingForClip;
        if(waitingForClip){
          setColor(0x28b531);
          instruction.arg2 = PRIME;
          instruction.send();
        } else {
          setColor(0);
          instruction.arg2 = CANCEL_PRIME;
          instruction.send();
        }
      } else {
        // show clip on screen, send request to load clip
        instruction.arg2 = REQUEST_CLIP;
        instruction.send();
      }
    }
    void handle(Instruction* i) {
      Serial.print(F("delegated to key: ")); Serial.print(index); Serial.print(" ");
      Serial.print("instruction code: "); Serial.println(i->instructionCode);
      switch (i->instructionCode) {
        case ARD_KEY_LED:
        {
          const char* hexCode = i->additionalArgs.c_str();
          color = naiveHexConversion(hexCode);
          extern Marco* mc;
          mc->controller->pixels->setPixelColor(index, color);
          Serial.println(color);
          break;
        }
        default:
          Serial.print(F("\"")); 
          Serial.print(i->instructionCode);
          Serial.println(F("\" is an unsupported instruction."));
      }
    }
    Clipboard(uint8_t idx)
    : Key(idx) {
    }
};

// DisplayConfiguration implementation
DisplayConfiguration::DisplayConfiguration(String header) {
    headerText = header;
}

// Controller implementation
Controller::Controller(Adafruit_NeoPixel* npx, Adafruit_SH1106G* ash, RotaryEncoder* re, DisplayConfiguration* dconf) {
  pixels = npx;
  display = ash;
  encoder = re;
  dc = dconf;

  // playStartupTone();
  // Create the neopixel strip with the built in definitions NUM_NEOPIXEL and PIN_NEOPIXEL
  // start pixels!
  pixels->begin();
  pixels->setBrightness(255);
  pixels->show(); // Initialize all pixels to 'off'

  setupDisplay();

  Keyboard.begin();
  Mouse.begin();
  
  encoderPos = encoder->getPosition();

  // set all mechanical keys to inputs
  for (uint8_t i=0; i<NUM_KEYS; i++) {
    pinMode(i+1, INPUT_PULLUP);
    keys[i] = new Clipboard(i);
  }

  iteration = 0;
}

void Controller::consumeSerial() {
  while(Serial.available() > 0) {
    static char message[MAX_MESSAGE_LENGTH];
    static unsigned int message_pos = 0;

    char inByte = Serial.read();

    //Message coming in (check not terminating character) and guard for over message size
    if ( inByte != '\n' && (message_pos < MAX_MESSAGE_LENGTH - 1) )
    {
      //Add the incoming byte to our message
      message[message_pos] = inByte;
      message_pos++;
    }
    //Full message received...
    else
    {
      //Add null character to string
      message[message_pos] = '\0';
      Instruction i(message, message_pos);
      Serial.println(i.instructionCode); Serial.println(i.callerIndex);
      Serial.println(i.additionalArgs.c_str());
      //Print the message (or do other things)
      Serial.println(message);
      delegateInstruction(&i);
      //Reset for the next message
      message_pos = 0;
    }
  }
}

void Controller::refresh() {
  consumeSerial();
  refreshDisplay();

  encoder->tick();          // check the encoder
  int tmp = encoder->getPosition();
  if (tmp > encoderPos) {
    Keyboard.press(KEY_UP_ARROW);
    delay(10);
    Keyboard.releaseAll();
  } else if (tmp < encoderPos) {
    Keyboard.press(KEY_DOWN_ARROW);
    delay(10);
    Keyboard.releaseAll();
  }
  encoderPos = tmp;

  for (int i=0; i<NUM_KEYS; i++) {
    if (!digitalRead(i+1) && !pressed[i]) { // switch pressed!
      pressed[i] = true;
      keys[i]->press();
      pixels->setPixelColor(i, keys[i]->color);  // make white
      // move the text into a 3x4 grid
      display->setCursor((i % 3)*48, 32 + (i/3)*8);
      display->print("KEY");
      display->print(i+1);
      // char strokes[] = {KEY_DELETE};
      // size_t elems = 1;
      // sendKeyCombo(strokes, elems);
    } else if (digitalRead(i+1)) {
      pressed[i] = false;
    }
  }

  // show neopixels, increment swirl
  pixels->show();
  iteration++;
}

Controller::~Controller() {
  Keyboard.end();
  Mouse.end();
  delete display;
  delete pixels;
  delete encoder;
}

void Controller::setupDisplay() {
  // Start OLED
  display->begin();
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
    Serial.println(F("Encoder button"));
    display->print(F("Encoder pressed "));
    pixels->setBrightness(255);     // bright!
  } else {
    pixels->setBrightness(80);
  }

  display->setCursor(0, 8);
  display->print(F("Rotary encoder: "));
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

void Controller::delegateInstruction(Instruction* i) {
  if(i->callerIndex < 0 || i->callerIndex > NUM_KEYS - 1) {
    Serial.print(F("invalid key index: ")); Serial.println(i->callerIndex);
    return;
  }
  keys[i->callerIndex]->handle(i);
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