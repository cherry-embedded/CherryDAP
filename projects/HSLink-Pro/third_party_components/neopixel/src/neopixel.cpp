//
// Created by yekai on 2025/3/11.
//

#include <cstdlib>
#include "neopixel.h"

NeoPixel::NeoPixel(NeoPixel::interface_type_t interfaceType, uint32_t pixel_cnt, uint8_t *buffer) {
    this->interfaceType = interfaceType;
    this->pixel_cnt = pixel_cnt;
    if (buffer == nullptr) {
        this->buffer = (uint8_t *) malloc(pixel_cnt * 3);
        buffer_is_from_malloc = true;
    } else {
        this->buffer = buffer;
        buffer_is_from_malloc = false;
    }
}

void NeoPixel::SetPixel(uint8_t pixel_idx, uint8_t r, uint8_t g, uint8_t b) {
    if (pixel_idx >= pixel_cnt) {
        return;
    }
    buffer[pixel_idx * 3] = g;
    buffer[pixel_idx * 3 + 1] = r;
    buffer[pixel_idx * 3 + 2] = b;
}

void NeoPixel::SetPixel(uint8_t pixel_idx, uint32_t color) {
    this->SetPixel(pixel_idx, (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);
}

NeoPixel::~NeoPixel() {
    if (buffer_is_from_malloc) {
        free(buffer);
    }
}
