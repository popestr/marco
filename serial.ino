#include "marco.h"

using namespace marco;

byte marco::hexCharToByte(char hexchar)
{
  // coerce lowercase
  if (hexchar >= 'a')
  {
    hexchar -= 'a' - 'A';
  }

  if (!(hexchar >= 'A' && hexchar <= 'F') && !(hexchar >= '0' && hexchar <= '9'))
  {
    return -1;
  }

  return (hexchar >= 'A') ? (hexchar - 'A' + 10) : (hexchar - '0');
}

uint32_t marco::intFromHexString(const char *hexCode, uint8_t digits)
{
  uint32_t output = 0x0;
  uint32_t mask = 0x0;
  for (uint8_t i = 0; i < digits; i++)
  {
    output = output | (hexCharToByte(hexCode[i]) << (4 * (digits - i - 1)));
    mask = mask | (0xF << 4 * (digits - i - 1));
  }
  Serial.print("hex converted to: ");
  Serial.println(output & mask);
  return output & mask;
}

uint32_t marco::intFromHexString(const char *hexCode)
{
  return intFromHexString(hexCode, DEFAULT_INSTRUCTION_HEX_DIGITS);
}

Instruction::Instruction(char instructionWithArgs[MAX_MESSAGE_LENGTH], int actualLength)
{
  Serial.print("actual length: ");
  Serial.println(actualLength);
  if (actualLength >= 12)
  {
    instructionCode = hexCharToByte(instructionWithArgs[8]);
    arg1 = hexCharToByte(instructionWithArgs[9]);
    arg2 = hexCharToByte(instructionWithArgs[10]);
    arg3 = hexCharToByte(instructionWithArgs[11]);
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
  this->sendf("%.4X");
}

// "%.4X" as fmt for 4-bit hex.
void Instruction::sendf(const char *fmt)
{
  char formatted[4];
  uint16_t fullInstruction =
      ((instructionCode & 0xF) << 16) |
      ((arg1 & 0xF) << 8) |
      ((arg2 & 0xF) << 4) |
      (arg3 & 0xF);

  sprintf(formatted, fmt, fullInstruction);

  Serial.print("[ARD::0x");
  Serial.print(formatted);
  Serial.println("]");
}
