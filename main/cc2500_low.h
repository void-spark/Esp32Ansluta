#pragma once

#include <stdlib.h>

void cc2500LowInit();

// Write value to CC2500 register at given address. Returns CC2500 status bye.
uint8_t writeRegister(uint8_t addr, uint8_t value);

uint8_t readStatusRegister(uint8_t addr);

// Send command strobe for given address. Returns CC2500 status bye.
uint8_t sendCommandStrobe(uint8_t addr);

// Enable underflow interrupt, to be called after configuring GDO0.
void enableUnderflowInterrupt();

// Wait until RX/TX underflow is signaled.
void waitForUnderflow();

// Print details of the given CC2500 status byte.
void printStatusByte(uint8_t status);
