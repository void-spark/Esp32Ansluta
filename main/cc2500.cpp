#include <string.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "spi.h"
#include "cc2500_def.h"
#include "cc2500_low.h"
#include "cc2500.h"

static const char *TAG = "cc2500";

// Reset defaults for all the registers.
const static uint8_t REG_DEFAULTS[] = {
    0x29, // REG_IOCFG2  
    0x2E, // REG_IOCFG1  
    0x3F, // REG_IOCFG0  
    0x07, // REG_FIFOTHR 
    0xD3, // REG_SYNC1   
    0x91, // REG_SYNC0   
    0xFF, // REG_PKTLEN  
    0x04, // REG_PKTCTRL1
    0x45, // REG_PKTCTRL0
    0x00, // REG_ADDR    
    0x00, // REG_CHANNR  
    0x0F, // REG_FSCTRL1 
    0x00, // REG_FSCTRL0 
    0x5E, // REG_FREQ2   
    0xC4, // REG_FREQ1   
    0xEC, // REG_FREQ0   
    0x8C, // REG_MDMCFG4 
    0x22, // REG_MDMCFG3 
    0x02, // REG_MDMCFG2 
    0x22, // REG_MDMCFG1 
    0xF8, // REG_MDMCFG0 
    0x47, // REG_DEVIATN 
    0x07, // REG_MCSM2   
    0x30, // REG_MCSM1   
    0x04, // REG_MCSM0   
    0x76, // REG_FOCCFG  
    0x6C, // REG_BSCFG   
    0x03, // REG_AGCTRL2 
    0x40, // REG_AGCTRL1 
    0x91, // REG_AGCTRL0 
    0x87, // REG_WOREVT1 
    0x6B, // REG_WOREVT0 
    0xF8, // REG_WORCTRL 
    0x56, // REG_FREND1  
    0x10, // REG_FREND0  
    0xA9, // REG_FSCAL3  
    0x0A, // REG_FSCAL2  
    0x20, // REG_FSCAL1  
    0x0D, // REG_FSCAL0  
    0x41, // REG_RCCTRL1 
    0x00, // REG_RCCTRL0 
    0x59, // REG_FSTEST  
    0x7F, // REG_PTEST   
    0x3F, // REG_AGCTEST 
    0x88, // REG_TEST2   
    0x31, // REG_TEST1   
    0x0B  // REG_TEST0   
};

// In memory storage of the register values we want to set
static uint8_t regConfig[sizeof(REG_DEFAULTS)];

#define BUFFER_SIZE 16

static uint8_t *bufferIn;
static uint8_t *bufferOut;

void cc2500LoadDefaults() {
    memcpy(regConfig, REG_DEFAULTS, sizeof(REG_DEFAULTS));
}

void cc2500LoadRegister(uint8_t addr, uint8_t value) {
    regConfig[addr] = value;
}

void configureFifoThreshold(fifoThreshold threshold) {
    cc2500LoadRegister(REG_FIFOTHR, threshold);
}

static esp_err_t configurePacketAutomationControl1(
    uint8_t preambleQualityEstimatorThreshold,
    bool crcAutoflush,
    bool appendedStatus,
    uint8_t addressCheck) {
    if(preambleQualityEstimatorThreshold > 0x07) {
        ESP_LOGE(TAG, "Preamble quality estimator threshold value too large");
        return ESP_FAIL;
    }
    if(addressCheck > 0x03) {
        ESP_LOGE(TAG, "Address check value too large");
        return ESP_FAIL;
    }
    uint8_t value = 
        preambleQualityEstimatorThreshold << 5 &
        crcAutoflush << 3 &
        appendedStatus << 2 &
        addressCheck;
    cc2500LoadRegister(REG_PKTCTRL1, value);
    return ESP_OK;
}

static esp_err_t configurePacketAutomationControl0(
    bool whiteData,
    uint8_t packetFormat,
    bool cc2400En, 
    bool crcEn, 
    uint8_t lengthConfig
    ) {
    if(packetFormat > 0x03) {
        ESP_LOGE(TAG, "Packet format value too large");
        return ESP_FAIL;
    }
    if(lengthConfig > 0x03) {
        ESP_LOGE(TAG, "Length config value too large");
        return ESP_FAIL;
    }
    uint8_t value = 
        whiteData << 6 &
        packetFormat << 4 &
        cc2400En << 3 &
        crcEn << 2 &
        lengthConfig;
    cc2500LoadRegister(REG_PKTCTRL1, value);
    return ESP_OK;
}

static esp_err_t configureDeviceAddress(uint8_t deviceAddress) {
    cc2500LoadRegister(REG_ADDR, deviceAddress);
    return ESP_OK;
}

static esp_err_t configureChannelNumber(uint8_t channelNumber) {
    cc2500LoadRegister(REG_CHANNR, channelNumber);
    return ESP_OK;
}

static esp_err_t configureFrequencyControlWord(uint32_t frequencyControlWord) {
    cc2500LoadRegister(REG_FREQ2, (frequencyControlWord >> 16) & 0xFF);
    cc2500LoadRegister(REG_FREQ1, (frequencyControlWord >> 8)  & 0xFF);
    cc2500LoadRegister(REG_FREQ0, frequencyControlWord & 0xFF);
    return ESP_OK;
}

void cc2500LoadCommon() {
    // GDO2 Low output drive strength, Active high,
    //      Asserts when sync word has been sent / received, and de-asserts at the end of the packet.
    //      In RX, the pin will de-assert when the optional address check fails or the RX FIFO overflows.
    //      In TX the pin will de-assert if the TX FIFO underflows.
    cc2500LoadRegister(REG_IOCFG2, 0x06);
    // GDO1: High impedance, we don't use it (as generic pin, still works as SO)
    cc2500LoadRegister(REG_IOCFG1, 0x2E);
    // GDO0: High impedance, we don't use it.
    cc2500LoadRegister(REG_IOCFG0, 0x2E);
    cc2500LoadRegister(REG_FSCTRL1, 0x09);
    configureFrequencyControlWord(6132657);
    cc2500LoadRegister(REG_MDMCFG4, 0x2D);
    cc2500LoadRegister(REG_MDMCFG3, 0x3B);
    cc2500LoadRegister(REG_MDMCFG2, 0x73); // MSK, 30/32 sync word bits detected
    cc2500LoadRegister(REG_MCSM0, 0x18); // Calibrate when going from IDLE to RX or TX (or FSTXON), Power on timeout expire count = 64
    cc2500LoadRegister(REG_FOCCFG, 0x1D);
    cc2500LoadRegister(REG_BSCFG, 0x1C);
    cc2500LoadRegister(REG_AGCTRL2, 0xC7);
    cc2500LoadRegister(REG_AGCTRL1, 0x00);
    cc2500LoadRegister(REG_AGCTRL0, 0xB2);
    cc2500LoadRegister(REG_FREND1, 0xB6);
    cc2500LoadRegister(REG_FREND0, 0x10);
    cc2500LoadRegister(REG_FSCAL3, 0xEA);
    cc2500LoadRegister(REG_FSCAL2, 0x0A);
    cc2500LoadRegister(REG_FSCAL1, 0x00);
    cc2500LoadRegister(REG_FSCAL0, 0x11);
}

void cc2500ApplyConfig() {
    cc2500LowWriteRegisters(REG_IOCFG2, regConfig, sizeof(regConfig));
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

esp_err_t cc2500Transmit(uint8_t *packet, size_t size) {
    if(size + 2 > BUFFER_SIZE) {
		ESP_LOGE(TAG, "Packet too large for buffer");
        return ESP_FAIL;
    }
    bufferOut[1] = size;
    memcpy(bufferOut + 2, packet, size);

    // We expect to be in IDLE state
    uint8_t status = cc2500LowSendCommandStrobe(CMD_SNOP);
    uint8_t state = GET_STATE(status);
    if(state != STATE_IDLE) {
        ESP_LOGW(TAG, "Invalid state@tx-pre: %d", state);
    }

    // Store the packet bytes in the TX FIFO
    spiChipEnable();
    spiExchangeBytes(bufferOut, bufferIn, size + 2);
    spiChipDisable();

    // Make sure no packet sent flag is still set.
    cc2500ResetUnderflow();

    // Start sending the packet
    cc2500LowSendCommandStrobe(CMD_STX);

    // Wait until packet is sent
    bool succes = cc2500LowWaitForUnderflow(25);
    if(!succes) {
        ESP_LOGW(TAG, "TX timeout");
    }

    // Once the packet is sent, we should automatically return to IDLE state.
    status = cc2500LowSendCommandStrobe(CMD_SNOP);
    state = GET_STATE(status);
    if(state != STATE_IDLE) {
        ESP_LOGW(TAG, "Invalid state@tx-post: %d", state);
    }

    return ESP_OK;
}

esp_err_t cc2500Init() {
    bufferIn = (uint8_t *)heap_caps_malloc(BUFFER_SIZE, MALLOC_CAP_DMA);
	memset(bufferIn, 0, BUFFER_SIZE);

    bufferOut = (uint8_t *)heap_caps_malloc(BUFFER_SIZE, MALLOC_CAP_DMA);
	memset(bufferOut, 0, BUFFER_SIZE);
    bufferOut[0] = REG_FIFO | HDR_BURST;

    cc2500LowInit();

	cc2500ResetDevice();

    cc2500ApplyConfig();
    cc2500LowWriteRegister(REG_PATABLE, 0xFF); // Set PATABLE[0]: Max Output power (for MSK)


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
