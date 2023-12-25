#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "spi.h"
#include "cc2500_def.h"
#include "cc2500_low.h"

#define PIN_NUM_GDO0 GPIO_NUM_5
#define PIN_NUM_GDO1 GPIO_NUM_2
#define PIN_NUM_GDO2 GPIO_NUM_3

static const char *TAG = "cc2500_low";

static EventGroupHandle_t event_group;

static const int gdo2Active = BIT0;

static void IRAM_ATTR gdo2IsrHandler(void* arg) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    BaseType_t xResult = xEventGroupSetBitsFromISR(event_group, gdo2Active, &xHigherPriorityTaskWoken);
    if(xResult == pdPASS){
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void cc2500LowInit() {
	spiInit();

    event_group = xEventGroupCreate();

    // Setup GDO2 as input pin
	ESP_ERROR_CHECK(gpio_reset_pin(PIN_NUM_GDO2));
	ESP_ERROR_CHECK(gpio_set_pull_mode(PIN_NUM_GDO2, GPIO_PULLUP_PULLDOWN));
	ESP_ERROR_CHECK(gpio_set_direction(PIN_NUM_GDO2, GPIO_MODE_INPUT));

    // Setup interrupt for GDO2
    ESP_ERROR_CHECK(gpio_set_intr_type(PIN_NUM_GDO2, GPIO_INTR_NEGEDGE));
    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    ESP_ERROR_CHECK(gpio_isr_handler_add(PIN_NUM_GDO2, gdo2IsrHandler, NULL));
}

uint8_t cc2500LowWriteRegister(uint8_t addr, uint8_t value) {
    if(addr > 0x3f) {
        ESP_LOGW(TAG, "Invalid register address %x", addr);
    }
    spiChipEnable();
	uint8_t status1 = spiExchangeByte(addr);
	uint8_t status2 = spiExchangeByte(value);
	spiChipDisable();

    return status2;
}

uint8_t cc2500LowWriteRegisters(uint8_t addr, uint8_t* buffer, size_t size) {
    if(addr > 0x3f) {
        ESP_LOGW(TAG, "Invalid register address %x", addr);
    }
    if(addr + size > 0x3f) {
        ESP_LOGW(TAG, "Invalid register address %x + size %x", addr, size);
    }
    spiChipEnable();
	uint8_t status1 = spiExchangeByte(addr | HDR_BURST);
    uint8_t status2 = 0x00;
    for(int pos = 0; pos < size; pos++) {
	    status2 = spiExchangeByte(buffer[pos]);
    }
	spiChipDisable();

    return status2;
}

uint8_t cc2500LowReadRegister(uint8_t addr) {
    if(addr > 0x3f) {
        ESP_LOGW(TAG, "Invalid register address %x", addr);
    }

    spiChipEnable();
	uint8_t status1 = spiExchangeByte(addr | HDR_READ);
	uint8_t result = spiExchangeByte(0x00);
	spiChipDisable();

    // cc2500LowPrintStatusByte(status1);

	return result;
}

void cc2500LowReadMultiRegisterValues(uint8_t addr, uint8_t* buffer, size_t size) {
    if(addr > 0x3f) {
        ESP_LOGW(TAG, "Invalid register address %x", addr);
    }
    spiChipEnable();
	uint8_t status1 = spiExchangeByte(addr | HDR_READ | HDR_BURST);
    for(int pos = 0; pos < size; pos++) {
	    buffer[pos] = spiExchangeByte(0x00);
    }
	spiChipDisable();

    return;
}


uint8_t cc2500LowReadStatusRegister(uint8_t addr) {
    if(addr > 0x3f) {
        ESP_LOGW(TAG, "Invalid status register address %x", addr);
    }

    spiChipEnable();
	uint8_t status1 = spiExchangeByte(addr | HDR_READ | HDR_BURST); // For status registers, burst bit needs to be set.
	uint8_t result = spiExchangeByte(0);
	spiChipDisable();

    // cc2500LowPrintStatusByte(status1);

	return result;
}

uint8_t cc2500LowSendCommandStrobe(uint8_t addr) {
    if(addr > 0x3f) {
        ESP_LOGW(TAG, "Invalid command strobe address %x", addr);
    }

    spiChipEnable();
	uint8_t status1 = spiExchangeByte(addr);
	spiChipDisable();

    return status1;
}

void cc2500ResetUnderflow() {
    xEventGroupClearBits(event_group, gdo2Active);
    return;
}

bool cc2500LowWaitForUnderflow(uint32_t timeoutMs) {
    EventBits_t bits = xEventGroupWaitBits(event_group, gdo2Active, true, true, timeoutMs / portTICK_PERIOD_MS);
    return bits != 0x00;
}

void cc2500LowPrintStatusByte(uint8_t status) {
    bool chipReady = !((status >> 7) & 0x01);
    uint8_t state = GET_STATE(status);
    uint8_t fifoBytesAvailable = status & 0x0f;
    ESP_LOGI(TAG, "Chip ready: %u, State: %u, Fifo bytes available: %u", chipReady, state, fifoBytesAvailable);
}
