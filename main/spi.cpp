#include <string.h>
#include "esp_err.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "spi.h"

#define SPI_HOST     SPI2_HOST

#define PIN_NUM_MISO GPIO_NUM_2
#define PIN_NUM_MOSI GPIO_NUM_7
#define PIN_NUM_CLK  GPIO_NUM_6
#define PIN_NUM_CS   GPIO_NUM_10

#define CLK_FREQ     5*1000*1000 // 5Mhz

static const char *TAG = "spi";

static spi_device_handle_t _handle;

void spiInit() {
    // Setup CS pin
	ESP_ERROR_CHECK(gpio_reset_pin(PIN_NUM_CS));
	ESP_ERROR_CHECK(gpio_set_direction(PIN_NUM_CS, GPIO_MODE_OUTPUT));
    spiChipDisable();

    // Setup SPI bus
    spi_bus_config_t buscfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .flags = SPICOMMON_BUSFLAG_MASTER | SPICOMMON_BUSFLAG_IOMUX_PINS | SPICOMMON_BUSFLAG_SCLK | SPICOMMON_BUSFLAG_MISO | SPICOMMON_BUSFLAG_MOSI
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));

    // Setup SPI device (CC2500)
    spi_device_interface_config_t devcfg = {
        .mode = 0,
        .clock_speed_hz = CLK_FREQ,
        .spics_io_num = -1, // we will use manual CS control
		.flags = SPI_DEVICE_NO_DUMMY,
        .queue_size = 7
    };
    ESP_ERROR_CHECK(spi_bus_add_device(SPI_HOST, &devcfg, &_handle));
}

void spiChipEnable() {
	ESP_ERROR_CHECK(gpio_set_level(PIN_NUM_CS, 0));
    while(gpio_get_level(PIN_NUM_MISO) > 0);
}

void spiChipDisable() {
	ESP_ERROR_CHECK(gpio_set_level(PIN_NUM_CS, 1));
}

uint8_t spiExchangeByte(uint8_t txValue) {
	spi_transaction_t spi_transaction;
	memset(&spi_transaction, 0, sizeof(spi_transaction_t));
    spi_transaction.flags = SPI_TRANS_USE_TXDATA | SPI_TRANS_USE_RXDATA;
	spi_transaction.length = 8;
    spi_transaction.tx_data[0] = txValue;
	ESP_ERROR_CHECK(spi_device_transmit(_handle, &spi_transaction));
	return spi_transaction.rx_data[0];
}

void spiExchangeBytes(uint8_t *bufferOut, uint8_t *bufferIn, size_t count) {
	spi_transaction_t spi_transaction;
	memset(&spi_transaction, 0, sizeof(spi_transaction_t));
    spi_transaction.flags = 0;
	spi_transaction.length = 8 * count;
    spi_transaction.tx_buffer = bufferOut;
    spi_transaction.rx_buffer = bufferIn;
	ESP_ERROR_CHECK(spi_device_transmit(_handle, &spi_transaction));
	return;
}
