//
// Created by yekai on 2025/3/23.
//

#include "crc32.h"
uint32_t CRC_CalcArray_Software(uint8_t *data, uint32_t len, uint32_t crc32) {
    const uint32_t st_const_value = 0x04c11db7;
    uint32_t crc_value = crc32;

    for (uint32_t i = 0; i < (len + 3) / 4; i++) {
        uint32_t xbit = 0x80000000;
        uint32_t word = 0;
        for (auto j = 0; j < 4; j++) {
            if (i * 4 + j < len) {
                word |= data[i * 4 + j] << (j * 8);
            }
        }
        for (uint32_t bits = 0; bits < 32; bits++) {
            if (crc_value & 0x80000000) {
                crc_value <<= 1;
                crc_value ^= st_const_value;
            } else {
                crc_value <<= 1;
            }
            if (word & xbit) {
                crc_value ^= st_const_value;
            }
            xbit >>= 1;
        }
    }
    return crc_value;
}