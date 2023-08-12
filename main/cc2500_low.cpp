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

static const int gdo0Active = BIT0;

static void IRAM_ATTR gdo0IsrHandler(void* arg) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    BaseType_t xResult = xEventGroupSetBitsFromISR(event_group, gdo0Active, &xHigherPriorityTaskWoken);
    if(xResult == pdPASS){
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void cc2500LowInit() {
	spiInit();

    event_group = xEventGroupCreate();

    // Setup interrupt for GDO1, which is also MISO!
    ESP_ERROR_CHECK(gpio_set_intr_type(PIN_NUM_GDO1, GPIO_INTR_NEGEDGE));

    ESP_ERROR_CHECK(gpio_install_isr_service(0));

    ESP_ERROR_CHECK(gpio_isr_handler_add(PIN_NUM_GDO1, gdo0IsrHandler, NULL));
    // The gpio_isr_handler_add method enables the interrupt, so disabling it should be done after
    ESP_ERROR_CHECK(gpio_intr_disable(PIN_NUM_GDO1));
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

void cc2500LowWaitForUnderflow() {
    ESP_ERROR_CHECK(gpio_intr_enable(PIN_NUM_GDO1));
    xEventGroupWaitBits(event_group, gdo0Active, true, true, portMAX_DELAY);
    ESP_ERROR_CHECK(gpio_intr_disable(PIN_NUM_GDO1));
    return;
}

void cc2500LowPrintStatusByte(uint8_t status) {
    bool chipReady = !((status >> 7) & 0x01);
    uint8_t state = (status >> 4) & 0x07;
    uint8_t fifoBytesAvailable = status & 0x0f;
    ESP_LOGI(TAG, "Chip ready: %u, State: %u, Fifo bytes available: %u", chipReady, state, fifoBytesAvailable);
}
