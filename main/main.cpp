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
#include "cc2500.h"
#include "ansluta.h"

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

static void buttonPressed() {
    uint16_t address = 0x3E94;
    ESP_ERROR_CHECK(gpio_set_level(PIN_NUM_LED1, 1));
    anslutaSendCommand(address, state + 1);
    ESP_ERROR_CHECK(gpio_set_level(PIN_NUM_LED1, 0));

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

    anslutaInit();

    esp_rom_gpio_pad_select_gpio(BTN_BOOT);
    gpio_set_direction(BTN_BOOT, GPIO_MODE_INPUT);
    
    buttonTimer = xTimerCreate("ButtonTimer", (5 / portTICK_PERIOD_MS), pdTRUE, (void *) 0, buttonTimerCallback);

    xTimerStart(buttonTimer, 0);

    printf("Minimum free heap size: %lu bytes\n", esp_get_minimum_free_heap_size());

    ESP_ERROR_CHECK(gpio_set_level(PIN_NUM_LED2, 1));

    while(true) {
        xEventGroupWaitBits(event_group, BUTTON_PRESSED, true, true, portMAX_DELAY);
        buttonPressed();
    }
}
