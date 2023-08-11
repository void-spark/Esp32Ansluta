#pragma once

#include <stdlib.h>

void cc2500LowInit();

// Write value to CC2500 register at given address. Returns CC2500 status bye.
uint8_t cc2500LowWriteRegister(uint8_t addr, uint8_t value);

uint8_t cc2500LowReadStatusRegister(uint8_t addr);

// Send command strobe for given address. Returns CC2500 status bye.
uint8_t cc2500LowSendCommandStrobe(uint8_t addr);

// Enable underflow interrupt, to be called after configuring GDO0.
void cc2500LowEnableUnderflowInterrupt();

// Wait until RX/TX underflow is signaled.
void cc2500LowWaitForUnderflow();

// Print details of the given CC2500 status byte.
void cc2500LowPrintStatusByte(uint8_t status);
