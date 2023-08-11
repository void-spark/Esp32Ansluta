#include "esp_err.h"
#include "esp_log.h"
#include "cc2500.h"
#include "ansluta.h"

#define PACKET_SIZE 7

static const char *TAG = "ansluta";

static uint8_t packet[PACKET_SIZE];

void anslutaInit() {
    cc2500Init();

    packet[0] = 0x06;
    packet[1] = 0x55;
    packet[2] = 0x01;
    // Two address bytes, one command byte
    packet[6] = 0xAA;
    // There used to be a 0xFF here, but it works without it?
}

void anslutaSendCommand(uint16_t address, uint8_t command) {
    packet[3] = (address >> 8);
    packet[4] = (address & 0xFF);
    packet[5] = command;

    for(int cnt = 0; cnt < 50; cnt++) {
        ESP_ERROR_CHECK(cc2500Transmit(packet, PACKET_SIZE));
    }
}
