#pragma once

#include <stdlib.h>

void livingcolorsInit();

void livingcolorsApplyConfig();

void livingcolorsSendCommand(uint8_t address[9], uint8_t command, uint8_t h, uint8_t s, uint8_t v);

void livingcolorsOff();

void livingcolorsOn();

void learnLamps();
