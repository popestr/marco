#include <Adafruit_NeoPixel.h>
#include <RotaryEncoder.h>
#include <Adafruit_SH110X.h>
#include <Keyboard.h>
#include <Mouse.h>
#include "marco.h"
#include <string>

#define PROG_CLIPBOARD 0x0

#define ARD_KEY_LED 0xF
#define SET_COLOR 0x0001
#define TURN_OFF 0x0002

#define OLED_DISPLAY 0xE
#define CLEAR 0x0000
#define SETTEXT 0x0001

#define TEXT_COLOR_NORMAL 0x0
#define TEXT_COLOR_INVERTED 0x0001

using namespace marco;

class Clipboard : public Key
{
public:
  void press()
  {
    Instruction instruction(PROG_CLIPBOARD, index);
    instruction.send();
  }
  Clipboard(uint8_t idx) : Key(idx)
  {
  }
};

DisplayRow::DisplayRow()
{
  this->text = std::string();
  this->inverted = false;
}

DisplayRow::DisplayRow(std::string text, bool inverted)
{
  this->text = text;
  this->inverted = inverted;
}

DisplayConfiguration::DisplayConfiguration(std::string header)
{
  lines[0] = DisplayRow(header, true);
  for (int lineNo = 1; lineNo < MAX_DISPLAY_TEXT_ROWS; lineNo++)
  {
    lines[lineNo] = DisplayRow();
  }
}

// DisplayConfiguration implementation
DisplayConfiguration::DisplayConfiguration(std::string header, std::string text)
    : DisplayConfiguration(header)
{
  setText(text, false, 0);
}

DisplayConfiguration::DisplayConfiguration(DisplayRow inputLines[9])
{
  for (uint8_t i = 0; i < 9; i++)
  {
    lines[i] = inputLines[i];
  }
}

void DisplayConfiguration::clear()
{
  for (int lineNo = 0; lineNo < MAX_DISPLAY_TEXT_ROWS; lineNo++)
  {
    lines[lineNo].text.erase();
  }
}

void DisplayConfiguration::setText(std::string text, bool inverted, int lineNo)
{
  size_t actualLength = text.length();
  int length = std::min(MAX_DISPLAY_TEXT_ROWS * MAX_DISPLAY_TEXT_WIDTH, int(actualLength));

  if (lineNo > MAX_DISPLAY_TEXT_ROWS - 1)
  {
    lineNo = MAX_DISPLAY_TEXT_ROWS - 1;
  }
  else if (lineNo < 0)
  {
    lineNo = 0;
  }

  for (int i = 0; i < length; i += MAX_DISPLAY_TEXT_WIDTH)
  {
    if (lineNo >= MAX_DISPLAY_TEXT_ROWS)
    {
      break;
    }
    lines[lineNo].text = std::string(&text[i], std::min(MAX_DISPLAY_TEXT_WIDTH, int(length - i)));
    lines[lineNo].inverted = inverted;
    lineNo++;
  }
}

// Controller implementation
Controller::Controller(Adafruit_NeoPixel *npx, Adafruit_SH1106G *ash, RotaryEncoder *re, DisplayConfiguration *dconf)
{
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
  for (uint8_t i = 0; i < NUM_KEYS; i++)
  {
    pinMode(i + 1, INPUT_PULLUP);
    keys[i] = new Clipboard(i);
  }

  iteration = 0;
}

void Controller::consumeSerial()
{
  while (Serial.available() > 0)
  {
    static char message[MAX_MESSAGE_LENGTH];
    static unsigned int message_pos = 0;

    char inByte = Serial.read();

    // Message coming in (check not terminating character) and guard for over message size
    if (inByte != '\n' && (message_pos < MAX_MESSAGE_LENGTH - 1))
    {
      // Add the incoming byte to our message
      message[message_pos] = inByte;
      message_pos++;
    }
    // Full message received...
    else
    {
      // Add null character to string
      message[message_pos] = '\0';
      Instruction i(message, message_pos);
      Serial.println(i.instructionCode);
      Serial.println(i.arg1);
      Serial.println(i.additionalArgs.c_str());
      Serial.println(message);
      handleInstruction(&i);
      // Reset for the next message
      message_pos = 0;
    }
  }
}

void Controller::handleInstruction(Instruction *i)
{
  switch (i->instructionCode)
  {
  case ARD_KEY_LED:
  {
    const char *hexCode = i->additionalArgs.c_str();
    keys[i->arg1]->color = naiveHexConversion(hexCode);
    // pixels->setPixelColor(i->arg1, keys[i->arg1]->color);
    break;
  }
  case OLED_DISPLAY:
  {
    switch (i->arg3)
    {
    case CLEAR:
      dc->clear();
      break;
    case SETTEXT:
      dc->setText(i->additionalArgs, i->arg1 > 0, i->arg2);
      break;
    }
    break;
  }
  default:
    Serial.print(F("\""));
    Serial.print(i->instructionCode);
    Serial.println(F("\" is an unsupported instruction."));
  }
}

void Controller::refresh()
{
  consumeSerial();
  refreshDisplay();

  encoder->tick(); // check the encoder
  int tmp = encoder->getPosition();
  if (tmp > encoderPos) // moved counter-clockwise
  {
    Keyboard.press(KEY_UP_ARROW);
    delay(10);
    Keyboard.releaseAll();
  }
  else if (tmp < encoderPos) // moved clockwise
  {
    Keyboard.press(KEY_DOWN_ARROW);
    delay(10);
    Keyboard.releaseAll();
  }
  encoderPos = tmp;

  for (int i = 0; i < NUM_KEYS; i++)
  {
    bool isPressed = !digitalRead(i + 1);
    if (isPressed && !pressed[i])
    { // switch pressed!
      pressed[i] = true;
      keys[i]->press();
      pixels->setPixelColor(i, keys[i]->color);
      // move the text into a 3x4 grid
      // char strokes[] = {KEY_DELETE};
      // size_t elems = 1;
      // sendKeyCombo(strokes, elems);
    }
    else if (!isPressed)
    {
      pressed[i] = false;
    }
  }

  // show neopixels, increment swirl
  pixels->show();
  iteration++;
}

Controller::~Controller()
{
  Keyboard.end();
  Mouse.end();
  delete display;
  delete pixels;
  delete encoder;
}

void Controller::setupDisplay()
{
  // Start OLED
  display->begin();
  display->display();

  // text display tests
  display->setTextSize(1);
  display->setTextWrap(false);
  display->setTextColor(SH110X_WHITE, SH110X_BLACK); // white text, black background
}

void Controller::refreshDisplay()
{
  display->clearDisplay();
  display->setCursor(0, 0);
  for (int i = 0; i < MAX_DISPLAY_TEXT_ROWS; i++)
  {
    if (dc->lines[i].inverted)
    {
      display->setTextColor(SH110X_BLACK, SH110X_WHITE);
    }
    else
    {
      display->setTextColor(SH110X_WHITE, SH110X_BLACK);
    }
    display->println(dc->lines[i].text.c_str());
  }
  // // check encoder press
  // display->setCursor(0, 24);
  // if (!digitalRead(PIN_SWITCH))
  // {
  //   Serial.println(F("Encoder button"));
  //   display->print(F("Encoder pressed "));
  //   pixels->setBrightness(255); // bright!
  // }
  // else
  // {
  //   pixels->setBrightness(80);
  // }

  // display->setCursor(0, 8);
  // display->print(F("Rotary encoder: "));
  // display->print(encoderPos);

  // display oled
  display->display();
}

void Controller::sendKeyCombo(char keys[], size_t elems)
{
  for (int i = 0; i < elems; i++)
  {
    Keyboard.press(keys[i]);
    delay(100);
  }
  Keyboard.releaseAll();
}

void Controller::playStartupTone()
{
  // Enable speaker
  pinMode(PIN_SPEAKER_ENABLE, OUTPUT);
  digitalWrite(PIN_SPEAKER_ENABLE, HIGH);
  // Play some tones
  pinMode(PIN_SPEAKER, OUTPUT);
  digitalWrite(PIN_SPEAKER, LOW);
  tone(PIN_SPEAKER, 988, 100); // tone1 - B5
  delay(100);
  tone(PIN_SPEAKER, 1319, 200); // tone2 - E6
  delay(200);
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Controller::Wheel(byte WheelPos)
{
  if (WheelPos < 85)
  {
    return pixels->Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  else if (WheelPos < 170)
  {
    WheelPos -= 85;
    return pixels->Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  else
  {
    WheelPos -= 170;
    return pixels->Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
}