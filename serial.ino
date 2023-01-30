#include "marco.h"

using namespace marco;

int marco::hexCharToInt(char hexchar) {
  return (hexchar >= 'A') ? (hexchar - 'A' + 10) : (hexchar - '0');
}

uint32_t marco::naiveHexConversion(const char* hexCode) {
  uint32_t output = 0x0;
  for(uint8_t i=0; i < 6; i++) {
    output = output | (hexCharToInt(hexCode[i]) << (4*i));
  }
  return output;
}

Instruction::Instruction(char instructionWithArgs[35]) {
  instructionCode = hexCharToInt(instructionWithArgs[10]);
  arg1 = hexCharToInt(instructionWithArgs[11]);
  arg2 = hexCharToInt(instructionWithArgs[12]);
  arg3 = hexCharToInt(instructionWithArgs[13]);
  additionalArgs = std::string(&instructionWithArgs[14], 22);
}

// sendInstruction builds a 16-bit program message to send to serial output.
// instructionCode occupies the first byte
// arg1, arg2, arg3 are single bytes, and are shifted into second, third, and fourth places.
// Example: sendInstruction(PROG_XXX, 0xF, 0xA, 0); where PROG_XXX is in 0x[0-F]000
//          results in [ARD::0xFFA0] output on serial.
void marco::sendInstruction(uint16_t instructionCode, uint8_t arg1, uint8_t arg2, uint8_t arg3) {
  char formatted[4];
  uint16_t fullInstruction = (instructionCode & 0xF000) | ((arg1 & 0xF) << 8) | ((arg2 & 0xF) << 4) | (arg3 & 0xF);

  sprintf(formatted, "%.4X", fullInstruction);
  
  Serial.print("[ARD::0x");
  Serial.print(formatted);
  Serial.println("]");
}

void marco::sendInstruction(uint16_t instructionCode, uint8_t arg1, uint8_t arg2) {
  sendInstruction(instructionCode, arg1, arg2, 0);
}

void marco::sendInstruction(uint16_t instructionCode, uint8_t arg1) {
  sendInstruction(instructionCode, arg1, 0, 0);
}

void marco::sendInstruction(uint16_t instructionCode) {
  sendInstruction(instructionCode, 0, 0, 0);
}
