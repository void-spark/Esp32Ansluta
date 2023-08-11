#include "esp_err.h"
#include "esp_log.h"
#include "spi.h"
#include "cc2500_def.h"
#include "cc2500_low.h"
#include "cc2500.h"

static const char *TAG = "cc2500";

// Configure the registers of the CC2500
static void cc2500Configure() {
    // GDO2: High impedance, we don't use it.
    cc2500LowWriteRegister(REG_IOCFG2, 0x2E);   

    // GDO0 Enable analog temperature sensor: false
    // GDO0 Active high  
    // GDO0: Asserts when sync word has been sent / received, and de-asserts at the end of the packet.
    //       In RX, the pin will de-assert when the optional address check fails or the RX FIFO overflows.
    //       In TX the pin will de-assert if the TX FIFO underflows.
    cc2500LowWriteRegister(REG_IOCFG0, 0x06);

    cc2500LowWriteRegister(REG_PKTLEN, 0xFF);
    cc2500LowWriteRegister(REG_PKTCTRL1, 0x04);
    cc2500LowWriteRegister(REG_PKTCTRL0, 0x05);
    cc2500LowWriteRegister(REG_ADDR, 0x01);
    cc2500LowWriteRegister(REG_CHANNR, 0x10);
    cc2500LowWriteRegister(REG_FSCTRL1, 0x09);
    cc2500LowWriteRegister(REG_FSCTRL0, 0x00);
    cc2500LowWriteRegister(REG_FREQ2, 0x5D);
    cc2500LowWriteRegister(REG_FREQ1, 0x93);
    cc2500LowWriteRegister(REG_FREQ0, 0xB1);
    cc2500LowWriteRegister(REG_MDMCFG4, 0x2D);
    cc2500LowWriteRegister(REG_MDMCFG3, 0x3B);
    cc2500LowWriteRegister(REG_MDMCFG2, 0x73); // MSK
    cc2500LowWriteRegister(REG_MDMCFG1, 0xA2);
    cc2500LowWriteRegister(REG_MDMCFG0, 0xF8);
    cc2500LowWriteRegister(REG_DEVIATN, 0x01);
    cc2500LowWriteRegister(REG_MCSM2, 0x07);
    
    // Clear channel indication: If RSSI below threshold unless currently receiving a packet (default)
    // Next state after finishing packet reception: IDLE (default)
    // Next state after finishing packet transmission: IDLE (default)
    cc2500LowWriteRegister(REG_MCSM1, 0x30);

    cc2500LowWriteRegister(REG_MCSM0, 0x18);
    cc2500LowWriteRegister(REG_FOCCFG, 0x1D);
    cc2500LowWriteRegister(REG_BSCFG, 0x1C);
    cc2500LowWriteRegister(REG_AGCTRL2, 0xC7);
    cc2500LowWriteRegister(REG_AGCTRL1, 0x00);
    cc2500LowWriteRegister(REG_AGCTRL0, 0xB2);
    cc2500LowWriteRegister(REG_WOREVT1, 0x87);
    cc2500LowWriteRegister(REG_WOREVT0, 0x6B);
    cc2500LowWriteRegister(REG_WORCTRL, 0xF8);
    cc2500LowWriteRegister(REG_FREND1, 0xB6);
    cc2500LowWriteRegister(REG_FREND0, 0x10);
    cc2500LowWriteRegister(REG_FSCAL3, 0xEA);
    cc2500LowWriteRegister(REG_FSCAL2, 0x0A);
    cc2500LowWriteRegister(REG_FSCAL1, 0x00);
    cc2500LowWriteRegister(REG_FSCAL0, 0x11);
    cc2500LowWriteRegister(REG_RCCTRL1, 0x41);
    cc2500LowWriteRegister(REG_RCCTRL0, 0x00);
    cc2500LowWriteRegister(REG_FSTEST, 0x59);

    cc2500LowWriteRegister(REG_TEST2, 0x88);
    cc2500LowWriteRegister(REG_TEST1, 0x31);
    cc2500LowWriteRegister(REG_TEST0, 0x0B);

    cc2500LowWriteRegister(REG_PATABLE, 0xFF); // Set PATABLE[0]: Max Output power (for MSK)
}

// Reset the CC2500
static uint8_t cc2500ResetDevice() {
	return cc2500LowSendCommandStrobe(CMD_SRES);
}

// Get the value of the CC2500 part number register.
static uint8_t cc2500GetPartNumber() {
	return cc2500LowReadStatusRegister(STATUS_PARTNUM);
}

// Get the value of the CC2500 version number register.
static uint8_t cc2500GetVersionNumber() {
	return cc2500LowReadStatusRegister(STATUS_VERSION);
}

esp_err_t cc2500Init() {
    cc2500LowInit();

	cc2500ResetDevice();

	cc2500Configure();

    cc2500LowEnableUnderflowInterrupt();

	uint8_t chipPart = cc2500GetPartNumber();
	uint8_t chipVersion = cc2500GetVersionNumber();
	ESP_LOGI(TAG, "Part number=%x", chipPart);
	ESP_LOGI(TAG, "Version number=%x", chipVersion);
	if (chipPart != 0x80 || chipVersion != 0x03) {
		ESP_LOGE(TAG, "CC2500 Not Installed");
		return ESP_FAIL;
	}
	ESP_LOGI(TAG, "CC2500 Installed");
	return ESP_OK;
}
