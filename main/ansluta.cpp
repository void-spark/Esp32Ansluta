#include "esp_err.h"
#include "esp_log.h"
#include "spi.h"
#include "cc2500_def.h"
#include "cc2500_low.h"
#include "cc2500.h"
#include "ansluta.h"

static const char *TAG = "ansluta";

void anslutaSendCommand(uint16_t address, uint8_t command) {
    for(int cnt = 0; cnt < 50; cnt++) {
        uint8_t status2 = cc2500LowSendCommandStrobe(CMD_SFTX);
        uint8_t state = (status2 >> 4) & 0x07;
        if(state != 0) {
            ESP_LOGW(TAG, "Invalid state: %d", state);
        }

        spiChipEnable();
        spiExchangeByte(REG_FIFO | HDR_BURST);
        spiExchangeByte(0x06);
        spiExchangeByte(0x55);
        spiExchangeByte(0x01);                 
        spiExchangeByte((uint8_t)(address >> 8));
        spiExchangeByte((uint8_t)(address & 0xFF));
        spiExchangeByte((uint8_t)command);
        spiExchangeByte(0xAA);
        spiExchangeByte(0xFF);
        spiChipDisable();

        cc2500LowSendCommandStrobe(CMD_STX);
        cc2500LowWaitForUnderflow();
    }
}
