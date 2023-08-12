#include "esp_err.h"
#include "esp_log.h"
#include "cc2500.h"
#include "cc2500_def.h"
#include "ansluta.h"

#define PACKET_SIZE 6

static const char *TAG = "ansluta";

static uint8_t packet[PACKET_SIZE];

void anslutaInit() {
    cc2500LoadDefaults();
    cc2500LoadCommon();
    cc2500LoadRegister(REG_PKTCTRL0, 0x05); // Ansluta does not use whitening
    cc2500LoadRegister(REG_CHANNR, 0x10); // Ansluta uses channel 10
    cc2500LoadRegister(REG_MDMCFG1, 0xA2); // Enable Forward Error Correction: enabled
    cc2500LoadRegister(REG_DEVIATN, 0x01);

    cc2500Init();

    packet[0] = 0x55;
    packet[1] = 0x01;
    // Two address bytes, one command byte
    packet[5] = 0xAA;
}

void anslutaSendCommand(uint16_t address, uint8_t command) {
    packet[2] = (address >> 8);
    packet[3] = (address & 0xFF);
    packet[4] = command;

    for(int cnt = 0; cnt < 50; cnt++) {
        ESP_ERROR_CHECK(cc2500Transmit(packet, PACKET_SIZE));
    }
}
