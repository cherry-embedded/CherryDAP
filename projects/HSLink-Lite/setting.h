#ifndef SETTING_H
#define SETTING_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    RESET_NRST = 0,
    RESET_POR,
    RESET_ARM_SWD_SOFT,
} Setting_ResetBit_t;

#define SETTING_CLEAR_RESET_MODE(reset, mode) (reset &= ~(1 << mode))
#define SETTING_SET_RESET_MODE(reset, mode) (reset |= (1 << mode))
#define SETTING_GET_RESET_MODE(reset, mode) (reset & (1 << mode))

typedef struct {
    uint8_t reset; //这是一个Bitmap，用来存储多种设置，每一位的功能见Setting_ResetBit_t
    bool jtag_20pin_compatible;
} HSLink_Setting_t;

extern HSLink_Setting_t HSLink_Setting;

#endif //SETTING_H
