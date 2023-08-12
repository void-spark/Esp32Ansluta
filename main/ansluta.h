#pragma once

#include <stdlib.h>

void anslutaInit();

void anslutaApplyConfig();

// Send a ansluta command with the given address.
// Command 0x01=Light OFF 0x02=50% 0x03=100% 0xFF=Pairing
void anslutaSendCommand(uint16_t address, uint8_t command);
