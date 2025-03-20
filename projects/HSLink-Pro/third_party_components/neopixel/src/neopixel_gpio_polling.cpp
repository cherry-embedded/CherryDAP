//
// Created by yekai on 2025/3/11.
//
#include "neopixel.h"

void NeoPixel_GPIO_Polling::Flush() {
    if (config.lock) {
        config.lock(config.user_data);
    }

    for (uint32_t j = 0; j < 3 * pixel_cnt; j++) {
        uint8_t byte = buffer[j];

        for (unsigned char i = 0; i < 8; i++) {
            if (byte & 0x80) {   // positive
                config.set_level(1, config.user_data);
                for (uint32_t i = 0; i < config.high_nop_cnt; i++) {
                    __asm volatile ("nop");
                }
                config.set_level(0, config.user_data);
                for (uint32_t i = 0; i < config.low_nop_cnt; i++) {
                    __asm volatile ("nop");
                }
            } else {    // negative
                config.set_level(1, config.user_data);
                for (uint32_t i = 0; i < config.low_nop_cnt; i++) {
                    __asm volatile ("nop");
                }
                config.set_level(0, config.user_data);
                for (uint32_t i = 0; i < config.high_nop_cnt; i++) {
                    __asm volatile ("nop");
                }
            }

            byte <<= 1;
        }
    }

    for (uint32_t i = 0; i < 75 * (config.high_nop_cnt + config.low_nop_cnt); i++) {
        __asm volatile ("nop");
    }

    if (config.unlock) {
        config.unlock(config.user_data);
    }
}
