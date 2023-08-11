#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_rom_gpio.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_https_ota.h"
#include "nvs_flash.h"
#include "lwip/apps/sntp.h"
#include "wifi_helper.h"
#include "mqtt_helper.h"
#include "spi.h"
#include "cc2500_def.h"
#include "cc2500_low.h"

static const char *TAG = "app";

#define BTN_BOOT GPIO_NUM_9

#define PIN_NUM_LED1 GPIO_NUM_12
#define PIN_NUM_LED2 GPIO_NUM_13

#define millis() xTaskGetTickCount()*portTICK_PERIOD_MS
#define delayMicroseconds(microsec) esp_rom_delay_us(microsec)

static const char* ota_url = "http://raspberrypi.fritz.box:8032/esp32/1602RawIdf.bin";

static TimerHandle_t buttonTimer;

static EventGroupHandle_t event_group;
static const int BUTTON_PRESSED = BIT0;

static uint8_t state = 0x02;

static void ota_task(void * pvParameter) {
    ESP_LOGI(TAG, "Starting OTA update...");

    esp_http_client_config_t config = {};
    config.url = ota_url;

    esp_https_ota_config_t ota_config = {};
    ota_config.http_config = &config;

    ESP_LOGI(TAG, "Attempting to download update from %s", config.url);
    esp_err_t ret = esp_https_ota(&ota_config);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "OTA Succeed, Rebooting...");
        esp_restart();
    } else {
        ESP_LOGE(TAG, "Firmware Upgrades Failed");
    }
    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

static void subscribeTopics() {
    subscribeDevTopic("$update");
    subscribeTopic("devices/cc2500");
}

static void handleMessage(const char* topic1, const char* topic2, const char* topic3, const char* data) {
    if(
        strcmp(topic1, "$update") == 0 && 
        topic2 == NULL && 
        topic3 == NULL
    ) {
        xTaskCreate(&ota_task, "ota_task", 8192, NULL, 5, NULL);
    }
}

static bool handleAnyMessage(const char* topic, const char* data) {

    if(strcmp(topic,"devices/cc2500") == 0) {


        return true;
    }

    return false;
}

static void sendAnslutaCommand(uint8_t command) {
    uint16_t address = 0x3E94;

    ESP_LOGI(TAG, "Sending...");

    for(int cnt = 0; cnt < 50; cnt++) {
        uint8_t status2 = sendCommandStrobe(CMD_SFTX);
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

        ESP_ERROR_CHECK(gpio_set_level(PIN_NUM_LED1, 1));
        sendCommandStrobe(CMD_STX);
        waitForUnderflow();
        ESP_ERROR_CHECK(gpio_set_level(PIN_NUM_LED1, 0));
    }

    ESP_LOGI(TAG, "Sent");
}

static void cc2500Init() {
    // writeRegister(REG_IOCFG2, 0x29); // CHIP_RDYn
    writeRegister(REG_IOCFG2, 0x2E);
    

    // GDO0 Enable analog temperature sensor: false
    // GDO0 Active high  
    // GDO0: Asserts when sync word has been sent / received, and de-asserts at the end of the packet.
    // In RX, the pin will de-assert when the optional address check fails or the RX FIFO overflows.
    // In TX the pin will de-assert if the TX FIFO underflows.
    writeRegister(REG_IOCFG0, 0x06);

    writeRegister(REG_PKTLEN, 0xFF);
    writeRegister(REG_PKTCTRL1, 0x04);
    writeRegister(REG_PKTCTRL0, 0x05);
    writeRegister(REG_ADDR, 0x01);
    writeRegister(REG_CHANNR, 0x10);
    writeRegister(REG_FSCTRL1, 0x09);
    writeRegister(REG_FSCTRL0, 0x00);
    writeRegister(REG_FREQ2, 0x5D);
    writeRegister(REG_FREQ1, 0x93);
    writeRegister(REG_FREQ0, 0xB1);
    writeRegister(REG_MDMCFG4, 0x2D);
    writeRegister(REG_MDMCFG3, 0x3B);
    writeRegister(REG_MDMCFG2, 0x73); // MSK
    writeRegister(REG_MDMCFG1, 0xA2);
    writeRegister(REG_MDMCFG0, 0xF8);
    writeRegister(REG_DEVIATN, 0x01);
    writeRegister(REG_MCSM2, 0x07);
    
    // Clear channel indication: If RSSI below threshold unless currently receiving a packet (default)
    // Next state after finishing packet reception: IDLE (default)
    // Next state after finishing packet transmission: IDLE (default)
    writeRegister(REG_MCSM1, 0x30);

    writeRegister(REG_MCSM0, 0x18);
    writeRegister(REG_FOCCFG, 0x1D);
    writeRegister(REG_BSCFG, 0x1C);
    writeRegister(REG_AGCTRL2, 0xC7);
    writeRegister(REG_AGCTRL1, 0x00);
    writeRegister(REG_AGCTRL0, 0xB2);
    writeRegister(REG_WOREVT1, 0x87);
    writeRegister(REG_WOREVT0, 0x6B);
    writeRegister(REG_WORCTRL, 0xF8);
    writeRegister(REG_FREND1, 0xB6);
    writeRegister(REG_FREND0, 0x10);
    writeRegister(REG_FSCAL3, 0xEA);
    writeRegister(REG_FSCAL2, 0x0A);
    writeRegister(REG_FSCAL1, 0x00);
    writeRegister(REG_FSCAL0, 0x11);
    writeRegister(REG_RCCTRL1, 0x41);
    writeRegister(REG_RCCTRL0, 0x00);
    writeRegister(REG_FSTEST, 0x59);

    writeRegister(REG_TEST2, 0x88);
    writeRegister(REG_TEST1, 0x31);
    writeRegister(REG_TEST0, 0x0B);

    writeRegister(REG_PATABLE, 0xFF); // Set PATABLE[0]: Max Output power (for MSK)
}

static void resetDevice(void) {
	sendCommandStrobe(CMD_SRES);
}

static uint8_t getPartNumber(void) {
	return readStatusRegister(STATUS_PARTNUM);
}

static uint8_t getVersionNumber(void) {
	return readStatusRegister(STATUS_VERSION);
}

static esp_err_t cc2500Begin() {
	spiInit();

    cc2500LowInit();

	resetDevice();

	cc2500Init();

    enableUnderflowInterrupt();

	uint8_t ChipPart = getPartNumber();
	uint8_t ChipVersion = getVersionNumber();
	ESP_LOGI(TAG, "Part number=%x", ChipPart);
	ESP_LOGI(TAG, "Version number=%x", ChipVersion);
	if (ChipPart != 0x80 || ChipVersion != 0x03) {
		ESP_LOGE(TAG, "CC2500 Not Installed");
		return ESP_FAIL;
	}
	ESP_LOGI(TAG, "CC2500 Installed");
	return ESP_OK;
}

static void buttonPressed() {
    // Command 0x01=Light OFF 0x02=50% 0x03=100% 0xFF=Pairing
    sendAnslutaCommand(state + 1);

    state = (state + 1) % 3;

    mqttPublish("devices/cc2500/button", "true", 4, 2, 0);
}

static void buttonTimerCallback(TimerHandle_t xTimer) { 
    // Active low!
    int level = gpio_get_level(BTN_BOOT);

    // https://www.embedded.com/electronics-blogs/break-points/4024981/My-favorite-software-debouncers
    static uint16_t state = 0; // Current debounce status
    state=(state<<1) | !level | 0xe000;
    if(state==0xf000) {
        xEventGroupSetBits(event_group, BUTTON_PRESSED);
    }
}

extern "C" void app_main() {

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    event_group = xEventGroupCreate();

	ESP_ERROR_CHECK(gpio_reset_pin(PIN_NUM_LED1));
	ESP_ERROR_CHECK(gpio_set_direction(PIN_NUM_LED1, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_level(PIN_NUM_LED1, 0));

	ESP_ERROR_CHECK(gpio_reset_pin(PIN_NUM_LED2));
	ESP_ERROR_CHECK(gpio_set_direction(PIN_NUM_LED2, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_level(PIN_NUM_LED2, 0));

    // Initialize WiFi
    wifiStart();

    ESP_LOGI(TAG, "Waiting for wifi");
    wifiWait();

    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();


    mqttStart(subscribeTopics, handleMessage, handleAnyMessage);

    ESP_LOGI(TAG, "Waiting for MQTT");

    mqttWait();

    cc2500Begin();

    esp_rom_gpio_pad_select_gpio(BTN_BOOT);
    gpio_set_direction(BTN_BOOT, GPIO_MODE_INPUT);
    
    buttonTimer = xTimerCreate("ButtonTimer", (5 / portTICK_PERIOD_MS), pdTRUE, (void *) 0, buttonTimerCallback);

    xTimerStart(buttonTimer, 0);

    printf("Minimum free heap size: %lu bytes\n", esp_get_minimum_free_heap_size());

    while(true) {
        xEventGroupWaitBits(event_group, BUTTON_PRESSED, true, true, portMAX_DELAY);
        buttonPressed();
    }
}
