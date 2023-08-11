#pragma once

#include <stdlib.h>

// Initialize SPI
void spiInit();

// Set CS active (low), wait for SO to go low.
void spiChipEnable();

// Set CS inactive (high).
void spiChipDisable();

// Send and read one byte.
uint8_t spiExchangeByte(uint8_t txValue);

// TEMP!
void spiEnableIntr();
