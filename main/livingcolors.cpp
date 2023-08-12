#include "esp_err.h"
#include "esp_log.h"
#include "cc2500.h"
#include "cc2500_def.h"
#include "livingcolors.h"

#define PACKET_SIZE 6

static const char *TAG = "livingcolors";

void livingcolorsInit() {
    cc2500LoadDefaults();
    cc2500LoadCommon();
    configureFifoThreshold(tx09_rx56);
    cc2500LoadRegister(REG_CHANNR, 0x03); // Channel used by LivingColors
    cc2500LoadRegister(REG_DEVIATN, 0x00);

    cc2500Init();
}
