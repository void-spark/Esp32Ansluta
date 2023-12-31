#pragma once

#include <stdlib.h>

void cc2500LowInit();

// Write value to CC2500 register at given address. Returns CC2500 status bye.
uint8_t cc2500LowWriteRegister(uint8_t addr, uint8_t value);

uint8_t cc2500LowWriteRegisters(uint8_t addr, uint8_t* buffer, size_t size);

uint8_t cc2500LowReadRegister(uint8_t addr);

void cc2500LowReadMultiRegisterValues(uint8_t addr, uint8_t* buffer, size_t size);

uint8_t cc2500LowReadStatusRegister(uint8_t addr);

// Send command strobe for given address. Returns CC2500 status bye.
uint8_t cc2500LowSendCommandStrobe(uint8_t addr);

// Reset the RX/TX underflow flag.
// Should be called before starting RX/TX, to ensure no flag is left from a earlier event. 
void cc2500ResetUnderflow();

// Wait until RX/TX underflow is signaled.
bool cc2500LowWaitForUnderflow(uint32_t timeoutMs);

// Print details of the given CC2500 status byte.
void cc2500LowPrintStatusByte(uint8_t status);
