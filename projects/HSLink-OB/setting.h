#ifndef SETTING_H
#define SETTING_H

#include <stdbool.h>
#include <stdint.h>
#include "board.h"

typedef enum {
    PORT_MODE_GPIO = 0,
    PORT_MODE_SPI = 1,
} PORT_Mode_t;

typedef struct {
    double vref;
    bool power_on;
    bool port_on;
} Setting_Power_t;

typedef enum {
    RESET_NRST = 0,
    RESET_POR,
    RESET_ARM_SWD_SOFT,
} Setting_ResetBit_t;

#define SETTING_CLEAR_RESET_MODE(reset, mode) (reset &= ~(1 << mode))
#define SETTING_SET_RESET_MODE(reset, mode) (reset |= (1 << mode))
#define SETTING_GET_RESET_MODE(reset, mode) (reset & (1 << mode))

typedef struct {
    uint32_t magic;
    bool boost;
    PORT_Mode_t swd_port_mode;
    PORT_Mode_t jtag_port_mode;
    bool jtag_single_bit_mode;
    Setting_Power_t power;
    uint8_t reset; //这是一个Bitmap，用来存储多种设置，每一位的功能见Setting_ResetBit_t
    bool led;
    uint8_t led_brightness;
    bool jtag_20pin_compatible;

    char nickname[128];
} HSLink_Setting_t;

typedef struct {
    bool reset_level;
} HSLink_Lazy_t;

static const uint32_t SETTING_MAGIC = 0xB7B7B7B7;

extern HSLink_Setting_t HSLink_Setting;

extern HSLink_Lazy_t HSLink_Global;

#endif //SETTING_H
