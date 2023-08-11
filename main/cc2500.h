#pragma once

#include <stdlib.h>

esp_err_t cc2500Init();
esp_err_t cc2500Transmit(uint8_t *packet, size_t size);
