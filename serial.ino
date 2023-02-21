#include "marco.h"

using namespace marco;

// #define INSTRUCTION_START_INDEX = 3
// #define APPROVED_CLIENT = 0xbf00

int marco::hexCharToInt(char hexchar)
{
  // if ascii ordering changes, we're fucked :(
  return (hexchar >= 'A') ? (hexchar - 'A' + 10) : (hexchar - '0');
}

uint32_t marco::naiveHexConversion(const char *hexCode)
{
  uint32_t output = 0x0;
  for (uint8_t i = 0; i < 6; i++)
  {
    output = output | (hexCharToInt(hexCode[i]) << (4 * i));
  }
  Serial.print("hex converted to: ");
  Serial.println(output & 0xFFFFFF);
  return output & 0xFFFFFF;
}

Instruction::Instruction(char instructionWithArgs[MAX_MESSAGE_LENGTH], int actualLength)
{
  Serial.print("actual length: ");
  +Serial.println(actualLength);
  if (actualLength >= 12)
  {
    instructionCode = hexCharToInt(instructionWithArgs[8]);
    arg1 = hexCharToInt(instructionWithArgs[9]);
    arg2 = hexCharToInt(instructionWithArgs[10]);
    arg3 = hexCharToInt(instructionWithArgs[11]);
    if (actualLength > 12)
    {
      additionalArgs = std::string(&instructionWithArgs[13], actualLength - 13);
    }
  }
  else
  {
    Serial.println(F("Input string too short for Instruction."));
  }
}

Instruction::Instruction(uint8_t instructionCode, uint8_t arg1, uint8_t arg2, uint8_t arg3)
{
  this->instructionCode = instructionCode;
  this->arg1 = arg1;
  this->arg2 = arg2;
  this->arg3 = arg3;
}

Instruction::Instruction(uint8_t instructionCode, uint8_t arg1, uint8_t arg2)
    : Instruction(instructionCode, arg1, arg2, 0) {}

Instruction::Instruction(uint8_t instructionCode, uint8_t arg1)
    : Instruction(instructionCode, arg1, 0, 0) {}

Instruction::Instruction(uint8_t instructionCode)
    : Instruction(instructionCode, 0, 0, 0) {}

uint16_t Instruction::serialize()
{
  return instructionCode << 4 * 3 | arg1 << 4 * 2 | arg2 << 4 | arg3;
}

void Instruction::send()
{
  // Serial.println(this->serialize());
  this->sendf("%.4X");
}

// "%.4X" as fmt for 4-bit hex.
void Instruction::sendf(char *fmt)
{
  char formatted[4];
  uint16_t fullInstruction = (instructionCode & 0xF000) | ((arg1 & 0xF) << 8) | ((arg2 & 0xF) << 4) | (arg3 & 0xF);

  sprintf(formatted, fmt, fullInstruction);

  Serial.print("[ARD::0x");
  Serial.print(formatted);
  Serial.println("]");
}
