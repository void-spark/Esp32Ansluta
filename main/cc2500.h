#pragma once

#include <stdlib.h>

enum fifoThreshold {   
    tx61_rx04 = 0x00,
    tx57_rx08 = 0x01,
    tx53_rx12 = 0x02,
    tx49_rx16 = 0x03,
    tx45_rx20 = 0x04,
    tx41_rx24 = 0x05,
    tx37_rx28 = 0x06,
    tx33_rx32 = 0x07,
    tx29_rx36 = 0x08,
    tx25_rx40 = 0x09,
    tx21_rx44 = 0x0A,
    tx17_rx48 = 0x0B,
    tx13_rx52 = 0x0C,
    tx09_rx56 = 0x0D,
    tx05_rx60 = 0x0E,
    tx01_rx64 = 0x0F
};

esp_err_t cc2500Init();

// Load default register values, does not apply anything.
void cc2500LoadDefaults();

// Load register values shared by LivingColors and Ansluta, does not apply anything.
void cc2500LoadCommon();

// Load the specified value to the specified register address, does not apply anything.
void cc2500LoadRegister(uint8_t addr, uint8_t value);

void cc2500ApplyConfig();

void configureFifoThreshold(fifoThreshold threshold);

esp_err_t cc2500Transmit(uint8_t *packet, size_t size);

