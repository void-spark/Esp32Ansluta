#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "cc2500.h"
#include "cc2500_def.h"
#include "livingcolors.h"


#include "cc2500_low.h"

#define PACKET_SIZE 14

static const char *TAG = "livingcolors";

static uint8_t sequence = 0;

static uint8_t packet[PACKET_SIZE];

static void livingcolorsConfig() {
    cc2500LoadDefaults();
    cc2500LoadCommon();
    configureFifoThreshold(tx09_rx56);
    cc2500LoadRegister(REG_CHANNR, 0x03); // Channel used by LivingColors
    cc2500LoadRegister(REG_DEVIATN, 0x00);
}

void livingcolorsInit() {
    livingcolorsConfig();
    cc2500Init();
}

void livingcolorsApplyConfig() {
    livingcolorsConfig();
    cc2500ApplyConfig();
}

void livingcolorsSendCommand(uint8_t dst[4], uint8_t src[4], uint8_t command, uint8_t h, uint8_t s, uint8_t v) {
    memcpy(packet, dst, 8);
    memcpy(packet + 4, src, 8);

    packet[8] = 0x11;

    packet[9] = command;

    packet[10] = sequence++;

    packet[11] = h;
    packet[12] = s;
    packet[13] = v;

    ESP_ERROR_CHECK(cc2500Transmit(packet, PACKET_SIZE));
}

void livingcolorsOff() {
    uint8_t src[4] = {0x50, 0xB2, 0x47, 0x66};
    uint8_t dst[4] = {0x73, 0xCD, 0x0D, 0x36};
    uint8_t command = 0x07;
    uint8_t h = 0x00;
    uint8_t s = 0x00;
    uint8_t v = 0x00;
    livingcolorsSendCommand(src, dst, command, h, s, v);
}


void livingcolorsOn() {
    uint8_t src[4] = {0x50, 0xB2, 0x47, 0x66};
    uint8_t dst[4] = {0x73, 0xCD, 0x0D, 0x36};
    uint8_t command = 0x05;
    uint8_t h = 0xFF;
    uint8_t s = 0x00;
    uint8_t v = 0xFF;
    livingcolorsSendCommand(src, dst, command, h, s, v);
}

void learnLamps() {
    const size_t capturecount = 50;
    uint8_t data[capturecount][17];

    for(int count = 0; count < capturecount;) {
        cc2500LowSendCommandStrobe(CMD_SRX);
        cc2500LowWaitForUnderflow();

        size_t packetLength = cc2500LowReadRegister(REG_FIFO) - 1;
        cc2500LowReadMultiRegisterValues(REG_FIFO, data[count++], packetLength + 2);
        cc2500LowSendCommandStrobe(CMD_SFRX);
    }

    for(int pos = 0 ; pos < capturecount; pos++) {
        printf("Data: ");
        for(int index = 0; index < 14; index++){
            printf("%02X ", data[pos][index]);
        }
        printf("\n");
        // ESP_LOGI(TAG, "RSSI: 0x%02X", data[pos][14]);
        // ESP_LOGI(TAG, "LQI: 0x%02X", data[pos][15]);
    }
}
