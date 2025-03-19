//
// Created by yekai on 2025/3/17.
//
#include <cstdio>
#include "neopixel.h"
#include "stdlib.h"
#include "hpm_spi.h"

NeoPixel_SPI_Polling::NeoPixel_SPI_Polling(uint32_t pixel_cnt, uint8_t *buffer, uint8_t *spi_buffer) :
        NeoPixel(interface_type_t::SPI_POLLING, pixel_cnt, buffer) {
    if (spi_buffer) {
        this->spi_buffer = spi_buffer;
        spi_buffer_is_from_malloc = false;
    } else {
        this->spi_buffer = (uint8_t *) malloc(pixel_cnt * 3 * 8 + 75);
        spi_buffer_is_from_malloc = true;
    }
    if (this->spi_buffer == nullptr) {
        printf("malloc failed\r\n");
    }
    memset(this->spi_buffer, 0, pixel_cnt * 3 * 8 + 75);
}

NeoPixel_SPI_Polling::~NeoPixel_SPI_Polling() {
    if (spi_buffer_is_from_malloc) {
        free(spi_buffer);
    }
}

void NeoPixel_SPI_Polling::Flush() {
    for (uint32_t j = 0; j < 3 * pixel_cnt; j++) {
        uint8_t byte = buffer[j];
        for (auto i = 0; i < 8; i++) {
            if (byte & 0x80) { // positive
                spi_buffer[j * 8 + i] = _bit1_pulse_width;
            } else {  // negative
                spi_buffer[j * 8 + i] = _bit0_pulse_width;
            }
            byte <<= 1;
        }
    }
    this->config.trans(spi_buffer, pixel_cnt * 3 * 8 + 75, config.user_data);
}
