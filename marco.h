#include <Adafruit_SH110X.h>
#include <string>

#ifndef _MARCO_H
#define _MARCO_H

#define MAX_MESSAGE_LENGTH 160
#define NUM_KEYS 12
#define MAX_DISPLAY_TEXT_WIDTH 21

namespace marco
{
  class DisplayConfiguration
  {
  public:
    std::string lines[7];
    std::string headerText;
    DisplayConfiguration(std::string header);
    DisplayConfiguration(std::string header, std::string text);
    void clear();
    void setText(std::string text);
    void setTextLine(std::string text, uint8_t lineNo);
  };
  class KeypressHandler
  {
  public:
    virtual void handle();
  };
  int hexCharToInt(char hexChar);
  uint16_t naiveHexConversion(const char *hexCode);
  class Instruction
  {
  public:
    uint8_t instructionCode;
    uint8_t callerIndex;
    uint8_t arg2;
    uint8_t arg3;
    std::string additionalArgs;
    Instruction(char instructionWithArgs[MAX_MESSAGE_LENGTH], int actualLength);
    Instruction(uint8_t instructionCode, uint8_t callerIndex, uint8_t arg2, uint8_t arg3);
    Instruction(uint8_t instructionCode, uint8_t callerIndex, uint8_t arg2);
    Instruction(uint8_t instructionCode, uint8_t callerIndex);
    Instruction(uint8_t instructionCode);
    uint16_t serialize();
    void send();
    void fsend(char *fmt);
  };
  class Key
  {
  public:
    uint8_t index;
    KeypressHandler *handler;
    uint32_t color;
    bool pressed;
    virtual void press();
    virtual void handle(Instruction *i);
    Key(uint8_t index);
    void setColor(uint32_t);
  };
  class Controller
  {
    // Hardware components
  public:
    Key *keys[12];
    Adafruit_NeoPixel *pixels; // key LEDs
    Adafruit_SH1106G *display;
    DisplayConfiguration *dc;
    RotaryEncoder *encoder;
    // Trackers
    int encoderPos;
    bool pressed[12];
    String headerText;
    uint8_t iteration;
    Controller(Adafruit_NeoPixel *npx, Adafruit_SH1106G *ash, RotaryEncoder *re, DisplayConfiguration *dconf);
    ~Controller();
    void consumeSerial();
    void refresh();

  private:
    void setupDisplay();
    void refreshDisplay();
    void sendKeyCombo(char keys[], size_t elems);
    void delegateInstruction(Instruction *i);
    void playStartupTone();
    uint32_t Wheel(byte WheelPos);
  };
}

#endif // _MARCO_H
