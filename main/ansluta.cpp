#include <string.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "spi.h"
#include "cc2500_def.h"
#include "cc2500_low.h"
#include "cc2500.h"
#include "ansluta.h"

static const char *TAG = "ansluta";

#define BUFFER_SIZE 12

static uint8_t *inBuffer;
static uint8_t *outBuffer;

void anslutaInit() {
    cc2500Init();

    inBuffer = (uint8_t *)heap_caps_malloc(BUFFER_SIZE, MALLOC_CAP_DMA);
	memset(inBuffer, 0, BUFFER_SIZE);

    outBuffer = (uint8_t *)heap_caps_malloc(BUFFER_SIZE, MALLOC_CAP_DMA);
	memset(outBuffer, 0,BUFFER_SIZE);

    outBuffer[0] = REG_FIFO | HDR_BURST;
    outBuffer[1] = 0x06;
    outBuffer[2] = 0x55;
    outBuffer[3] = 0x01;
    outBuffer[7] = 0xAA;
    outBuffer[8] = 0xFF;
}

void anslutaSendCommand(uint16_t address, uint8_t command) {
    outBuffer[4] = (address >> 8);
    outBuffer[5] = (address & 0xFF);
    outBuffer[6] = command;

    for(int cnt = 0; cnt < 50; cnt++) {
        uint8_t status = cc2500LowSendCommandStrobe(CMD_SFTX);
        uint8_t state = (status >> 4) & 0x07;
        if(state != 0) {
            ESP_LOGW(TAG, "Invalid state: %d", state);
        }

        spiChipEnable();
        spiExchangeBytes(outBuffer, inBuffer, 9);
        spiChipDisable();

        cc2500LowSendCommandStrobe(CMD_STX);
        cc2500LowWaitForUnderflow();
    }
}
