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

// Send and read mulktiple bytes, buffers must be DMA compatible and 32 bit aligned (start and end).
void spiExchangeBytes(uint8_t *bufferOut, uint8_t *bufferIn, size_t count);
