#include <Adafruit_SH110X.h>
#include <string>

#ifndef _MARCO_H
#define _MARCO_H

#define MAX_MESSAGE_LENGTH 165
#define NUM_KEYS 12
#define MAX_DISPLAY_TEXT_WIDTH 21
#define MAX_DISPLAY_TEXT_ROWS 8

#define DEFAULT_INSTRUCTION_HEX_DIGITS 6

namespace marco
{
  class DisplayRow
  {
  public:
    std::string text;
    bool inverted;
    DisplayRow();
    DisplayRow(std::string text, bool inverted);
  };
  class DisplayConfiguration
  {
  public:
    DisplayRow lines[MAX_DISPLAY_TEXT_ROWS];
    DisplayConfiguration(std::string header);
    DisplayConfiguration(std::string header, std::string text);
    DisplayConfiguration(DisplayRow inputLines[MAX_DISPLAY_TEXT_ROWS]);
    void clear();
    void clear(uint8_t startLine, uint8_t endLine);
    void setText(std::string text, bool inverted, int lineNo);
  };
  class MenuRow
  {
  public:
    DisplayRow displayRow;
    void invoke();
    MenuRow(std::string text, bool selected);
  };
  class MenuConfiguration
  {
  public:
    MenuRow rows[10];
    uint8_t selectedRow;
    DisplayRow *toConfig();
  };
  class KeypressHandler
  {
  public:
    virtual void handle();
  };
  int hexCharToInt(char hexChar);
  uint32_t naiveHexConversion(const char *hexCode);
  uint32_t naiveHexConversion(const char *hexCode, uint8_t digits);
  class Instruction
  {
  public:
    uint8_t instructionCode;
    uint8_t arg1;
    uint8_t arg2;
    uint8_t arg3;
    std::string additionalArgs;
    Instruction(char instructionWithArgs[MAX_MESSAGE_LENGTH], int actualLength);
    Instruction(uint8_t instructionCode, uint8_t arg1, uint8_t arg2, uint8_t arg3);
    Instruction(uint8_t instructionCode, uint8_t arg1, uint8_t arg2);
    Instruction(uint8_t instructionCode, uint8_t arg1);
    Instruction(uint8_t instructionCode);
    uint16_t serialize();
    void send();
    void sendf(const char *fmt);
  };
  class Key
  {
  public:
    uint8_t index;
    KeypressHandler *handler;
    uint32_t color;
    bool pressed;
    unsigned long lastPressMillis;
    unsigned long pressDuration;
    uint8_t lastTransmittedDuration;
    virtual void press();
    virtual void unpress();
    Key(uint8_t index);
    void setColor(uint32_t);
    uint8_t durationToByte();
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
    String headerText;
    uint8_t iteration;
    Controller(Adafruit_NeoPixel *npx, Adafruit_SH1106G *ash, RotaryEncoder *re, DisplayConfiguration *dconf);
    ~Controller();
    void consumeSerial();
    void handleInstruction(Instruction *i);
    void refresh();

  private:
    void setupDisplay();
    void refreshDisplay();
    void sendKeyCombo(char keys[], size_t elems);
    void playStartupTone();
    uint32_t Wheel(byte WheelPos);
  };
}

#endif // _MARCO_H
