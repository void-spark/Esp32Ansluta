#pragma once

#include <stdlib.h>

void livingcolorsInit();

void livingcolorsApplyConfig();

bool livingcolorsSendCommand(uint8_t address[9], uint8_t command, uint8_t h, uint8_t s, uint8_t v, bool checkResponse);

bool livingcolorsOff(bool checkResponse);

bool livingcolorsOn(bool checkResponse);

bool livingcolorsHsv(uint8_t h, uint8_t s, uint8_t v, bool checkResponse);

bool livingcolorsRgb(uint8_t r, uint8_t g, uint8_t b, bool checkResponse);

void learnLamps();
