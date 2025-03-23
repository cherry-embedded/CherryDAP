//
// Created by yekai on 2025/3/11.
//

#include <cstdlib>
#include <cstdio>
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
    if (this->buffer == nullptr) {
        printf("malloc failed\n");
    }
    memset(this->buffer, 0, pixel_cnt * 3);
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

void NeoPixel::SetPixel(uint8_t pixel_idx, const Color &color) {
    if (pixel_idx >= pixel_cnt) {
        return;
    }
    buffer[pixel_idx * 3] = color.g;
    buffer[pixel_idx * 3 + 1] = color.r;
    buffer[pixel_idx * 3 + 2] = color.b;
}

void NeoPixel::ModifyPixel(uint8_t pixel_idx, NeoPixel::color_type_t color, uint8_t value) {
    if (pixel_idx >= pixel_cnt) {
        return;
    }
//    printf("ModifyPixel before %d %d %d\n", buffer[pixel_idx * 3], buffer[pixel_idx * 3 + 1], buffer[pixel_idx * 3 + 2]);
    switch (color) {
        case NeoPixel::color_type_t::COLOR_R:
            buffer[pixel_idx * 3 + 1] = value;
            break;
        case NeoPixel::color_type_t::COLOR_G:
            buffer[pixel_idx * 3] = value;
            break;
        case NeoPixel::color_type_t::COLOR_B:
            buffer[pixel_idx * 3 + 2] = value;
            break;
    }
//    printf("ModifyPixel after %d %d %d\n", buffer[pixel_idx * 3], buffer[pixel_idx * 3 + 1], buffer[pixel_idx * 3 + 2]);
}

NeoPixel::~NeoPixel() {
    if (buffer_is_from_malloc) {
        free(buffer);
    }
}
