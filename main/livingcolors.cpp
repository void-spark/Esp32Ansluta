#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_err.h"
#include "esp_log.h"
#include "cc2500.h"
#include "cc2500_def.h"
#include "livingcolors.h"


#include "cc2500_low.h"

#define PACKET_SIZE 14

static const char *TAG = "livingcolors";

static uint8_t sequence = 0;

static uint8_t packet[PACKET_SIZE];

static void livingcolorsConfig() {
    cc2500LoadDefaults();
    cc2500LoadCommon();
    configureFifoThreshold(tx09_rx56);
    cc2500LoadRegister(REG_CHANNR, 0x03); // Channel used by LivingColors
    cc2500LoadRegister(REG_DEVIATN, 0x00);
}

void livingcolorsInit() {
    livingcolorsConfig();
    cc2500Init();
}

void livingcolorsApplyConfig() {
    livingcolorsConfig();
    cc2500ApplyConfig();
}

void livingcolorsSendCommand(uint8_t dst[4], uint8_t src[4], uint8_t command, uint8_t h, uint8_t s, uint8_t v) {
    memcpy(packet, dst, 4);
    memcpy(packet + 4, src, 4);

    packet[8] = 0x11;

    packet[9] = command;

    packet[10] = sequence++;

    packet[11] = h;
    packet[12] = s;
    packet[13] = v;

    for(int attempt = 0; attempt < 20; attempt++) {
        ESP_ERROR_CHECK(cc2500Transmit(packet, PACKET_SIZE));

        size_t receive_buffer_size = 16;

        uint8_t receive_buffer[receive_buffer_size] = {};

        bool ok = true;

        cc2500LowSendCommandStrobe(CMD_SRX);
        cc2500LowWaitForUnderflow();
        size_t packetLength = cc2500LowReadRegister(REG_FIFO);
        // +2 for the 2 quality related bytes
        if(packetLength + 2 == receive_buffer_size) {
            cc2500LowReadMultiRegisterValues(REG_FIFO, receive_buffer, packetLength + 2);
        } else {
            ok = false;
        }
        cc2500LowSendCommandStrobe(CMD_SFRX);

        if( memcmp (receive_buffer + 0, src, 4) != 0) {
            ok = false;
        }
        if( memcmp (receive_buffer + 4, dst, 4) != 0) {
            ok = false;
        }
        if(receive_buffer[8] != 0x11) {
            ok = false;
        }
        if(receive_buffer[9] != command +1 && receive_buffer[9] != command) {
            ok = false;
        }

        if(!ok) {
            vTaskDelay(50 / portTICK_PERIOD_MS);
        } else {
            break;
        }
    }
}

void livingcolorsOff() {
    uint8_t src[4] = {0x50, 0xB2, 0x47, 0x66};
    uint8_t dst[4] = {0x73, 0xCD, 0x0D, 0x36};
    uint8_t command = 0x07;
    uint8_t h = 0x00;
    uint8_t s = 0x00;
    uint8_t v = 0x00;
    livingcolorsSendCommand(src, dst, command, h, s, v);
}


void livingcolorsOn() {
    uint8_t src[4] = {0x50, 0xB2, 0x47, 0x66};
    uint8_t dst[4] = {0x73, 0xCD, 0x0D, 0x36};
    uint8_t command = 0x05;
    uint8_t h = 0xFF;
    uint8_t s = 0x00;
    uint8_t v = 0xFF;
    livingcolorsSendCommand(src, dst, command, h, s, v);    
}

#define MIN3(x,y,z) ((y)<=(z)?((x)<=(y)?(x):(y)):((x)<=(z)?(x):(z)))
#define MAX3(x,y,z) ((y)>=(z)?((x)>=(y)?(x):(y)):((x)>=(z)?(x):(z)))

static void convertRGBtoHSV(uint8_t r, uint8_t g, uint8_t b, uint8_t& h, uint8_t& s, uint8_t& v) {
    unsigned char rgb_min = MIN3(r, g, b);
    unsigned char rgb_max = MAX3(r, g, b);

    v = rgb_max;

    if (v == 0) {
        // colour is black
        h = s = 0;
        return;
    }

    s = 255 * long(rgb_max - rgb_min) / v;

    if (s == 0) {
        // grey colour, no chroma
        h = 0;
        return;
    }

    // compute hue
    if (rgb_max == r) {
        h = 0 + 43 * (g - b) / (rgb_max - rgb_min);
    } else if (rgb_max == g) {
        h = 85 + 43 * (b - r) / (rgb_max - rgb_min);
    } else {
        // rgb_max == b
        h = 171 + 43 * (r - g) / (rgb_max - rgb_min);
    }
}

//Conversion table which maps 'real' hue values to the funky ones the
//LivingColors-system seems to use.
const uint8_t hue_convtab[]={
    2, 4, 5, 7, 9, 10, 12, 14, 
    15, 17, 19, 20, 22, 24, 25, 27, 
    29, 30, 32, 34, 35, 37, 39, 40, 
    42, 44, 45, 47, 49, 50, 52, 54, 
    55, 57, 59, 60, 62, 64, 65, 67, 
    69, 70, 72, 73, 74, 75, 75, 76, 
    77, 77, 78, 79, 79, 80, 81, 81, 
    82, 83, 83, 84, 85, 85, 86, 87, 
    88, 88, 89, 90, 90, 91, 92, 92, 
    93, 94, 94, 95, 96, 96, 97, 98, 
    98, 99, 100, 100, 101, 102, 103, 104, 
    105, 106, 107, 108, 110, 111, 112, 113, 
    114, 115, 116, 117, 119, 120, 121, 122, 
    123, 124, 125, 126, 128, 129, 130, 131, 
    132, 133, 134, 135, 137, 138, 139, 140, 
    141, 142, 143, 144, 146, 147, 148, 149, 
    150, 151, 152, 153, 153, 154, 155, 156, 
    157, 158, 158, 159, 160, 161, 162, 163, 
    164, 164, 165, 166, 167, 168, 169, 169, 
    170, 171, 172, 173, 174, 174, 175, 176, 
    177, 178, 179, 180, 180, 181, 182, 183, 
    184, 185, 185, 186, 187, 187, 187, 188, 
    188, 189, 189, 190, 190, 191, 191, 191, 
    192, 192, 193, 193, 194, 194, 195, 195, 
    196, 196, 196, 197, 197, 198, 198, 199, 
    199, 200, 200, 200, 201, 201, 202, 202, 
    203, 203, 204, 204, 204, 205, 206, 207, 
    208, 210, 211, 212, 213, 215, 216, 217, 
    218, 219, 221, 222, 223, 224, 226, 227, 
    228, 229, 231, 232, 233, 234, 236, 237, 
    238, 239, 241, 242, 243, 244, 246, 247, 
    248, 249, 251, 252, 253, 254, 255, 1 };

//The LivingColors idea of hue differs a bit from the hue everyone else;
//while the r->y->g->c->b->m->r-idea persists, it's not completely
//linear. The non-linearity is solved by looking the hue to send up in a
//lookuptable 
uint8_t tweakHue(uint8_t realhue) {
    return hue_convtab[realhue];
}

void livingcolorsHsv(uint8_t h, uint8_t s, uint8_t v) {
    uint8_t src[4] = {0x50, 0xB2, 0x47, 0x66};
    uint8_t dst[4] = {0x73, 0xCD, 0x0D, 0x36};
    uint8_t command = 0x03;
    livingcolorsSendCommand(src, dst, command, h, s, v);    
}

void livingcolorsRgb(uint8_t r, uint8_t g, uint8_t b) {
    uint8_t src[4] = {0x50, 0xB2, 0x47, 0x66};
    uint8_t dst[4] = {0x73, 0xCD, 0x0D, 0x36};
    uint8_t command = 0x03;
    uint8_t h = 0;
    uint8_t s = 0;
    uint8_t v = 0;
    convertRGBtoHSV(r, g, b, h, s, v);
    h = tweakHue(h);
    livingcolorsSendCommand(src, dst, command, h, s, v);    
}

void learnLamps() {
    const size_t capturecount = 50;
    uint8_t data[capturecount][17];
    int64_t timings[capturecount];

    for(int count = 0; count < capturecount; count++) {
        cc2500LowSendCommandStrobe(CMD_SRX);
        cc2500LowWaitForUnderflow();   
        timings[count] = esp_timer_get_time();

        size_t packetLength = cc2500LowReadRegister(REG_FIFO);
        cc2500LowReadMultiRegisterValues(REG_FIFO, data[count], packetLength + 2);
        cc2500LowSendCommandStrobe(CMD_SFRX);
    }

    for(int pos = 0 ; pos < capturecount; pos++) {
        printf("Data: %lld ", timings[pos]);
        for(int index = 0; index < 14; index++){
            printf("%02X ", data[pos][index]);
        }
        printf("\n");
        // ESP_LOGI(TAG, "RSSI: 0x%02X", data[pos][14]);
        // ESP_LOGI(TAG, "LQI: 0x%02X", data[pos][15]);
    }
}
