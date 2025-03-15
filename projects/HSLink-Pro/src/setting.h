#ifndef SETTING_H
#define SETTING_H

#include <stdbool.h>
#include <stdint.h>
#include "BL_Setting_Common.h"

typedef enum {
    PORT_MODE_GPIO = 0,
    PORT_MODE_SPI = 1,
} PORT_Mode_t;

typedef struct {
    double voltage;
    bool power_on;
    bool port_on;
} Setting_Power_t;

typedef enum {
    RESET_NRST = 0,
    RESET_POR,
    RESET_ARM_SWD_SOFT,
} Setting_ResetBit_t;

typedef struct {
    uint8_t major;
    uint8_t minor;
    uint8_t patch;
    uint8_t reserved;
} Setting_Version_t;

#define SETTING_CLEAR_RESET_MODE(reset, mode) (reset &= ~(1 << mode))
#define SETTING_SET_RESET_MODE(reset, mode) (reset |= (1 << mode))
#define SETTING_GET_RESET_MODE(reset, mode) (reset & (1 << mode))

typedef struct {
    uint32_t magic;
    bool boost;
    PORT_Mode_t swd_port_mode;
    PORT_Mode_t jtag_port_mode;
    Setting_Power_t power;
    uint8_t reset; //这是一个Bitmap，用来存储多种设置，每一位的功能见Setting_ResetBit_t
    bool led;
    uint8_t led_brightness;

    char nickname[128];
} HSLink_Setting_t;

static const uint32_t SETTING_MAGIC = 0xB7B7B7B7;

extern HSLink_Setting_t HSLink_Setting;
extern Setting_Version_t HSLink_Hardware_Version;
extern BL_Setting_t bl_setting;

#ifdef __cplusplus
extern "C"
{
#endif

void Setting_Init(void);

void Setting_Save(void);

void Setting_SaveHardwareVersion(Setting_Version_t hw_ver);

/**
 * @brief 比较硬件版本号是否相同，如果输入的是UINT8_MAX，则不比较这一位以及后面的版本号
 * @return 如果硬件版本号相同，返回true，否则返回false
 */
uint8_t Setting_IsHardwareVersion(uint8_t major, uint8_t minor, uint8_t patch);

uint8_t Setting_GetSRSTLevel(void);

#ifdef __cplusplus
}
#endif

#endif //SETTING_H
