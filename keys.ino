#include "marco.h"

using namespace marco;

Key::Key(uint8_t idx)
{
  index = idx;
  pressDuration = 0;
}

void Key::setColor(uint32_t c)
{
  color = c;
}

void Key::press()
{
  if (!pressed)
  {
    pressed = true;
    lastPressMillis = millis();
    Instruction keydown(EVENT, index, KEYDOWN);
    keydown.send();
  }
  else
  {
    pressDuration = millis() - lastPressMillis;
    uint8_t pressDuration100Ms = durationToByte();
    if (pressDuration100Ms != lastTransmittedDuration)
    {
      lastTransmittedDuration = pressDuration100Ms;
      Instruction keyhold(EVENT, index, KEYHOLD, pressDuration100Ms);
      keyhold.send();
    }
  }
}

void Key::unpress()
{
  pressed = false;
  pressDuration = 0;

  Instruction keyup(EVENT, index, KEYUP, durationToByte());
  keyup.send();
}

uint8_t Key::durationToByte()
{
  return uint8_t((pressDuration / (unsigned long)100) % (0xF + 1));
}