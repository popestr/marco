#include <Adafruit_SH110X.h>
#include <string>

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
      virtual void press();
      virtual void handle(char instructionWithArgs[35]);
      Key(uint8_t index);
      void setColor(uint32_t);
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
    bool pressed[12];
    String headerText;
    uint8_t iteration;
    public:
      Controller(Adafruit_NeoPixel* npx, Adafruit_SH1106G* ash, RotaryEncoder* re, DisplayConfiguration* dconf);
      ~Controller();
      void consumeSerial();
      void refresh();
    private:
      void setupDisplay();
      void refreshDisplay();
      void sendKeyCombo(char keys[], size_t elems);
      void delegateInstruction(char instructionWithArgs[35]);
      void playStartupTone();
      uint32_t Wheel(byte WheelPos);
  };
  int hexCharToInt(char hexChar);
  uint32_t naiveHexConversion(const char* hexCode);
  class Instruction {
    public:
      uint8_t instructionCode;
      uint8_t arg1;
      uint8_t arg2;
      uint8_t arg3;
      std::string additionalArgs;
      Instruction(char instructionWithArgs[35]);
  };
  void sendInstruction(uint16_t instructionCode, uint8_t arg1, uint8_t arg2, uint8_t arg3);
  void sendInstruction(uint16_t instructionCode, uint8_t arg1, uint8_t arg2);
  void sendInstruction(uint16_t instructionCode, uint8_t arg1);
  void sendInstruction(uint16_t instructionCode);
}

#endif // _MARCO_H