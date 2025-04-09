/*
 * Copyright (c) 2024 HalfSweet
 *
 * 参考硬件设计：https://github.com/HalfSweet/HSLinkPro-Hardware
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef HPM5300EVKLITE_DAP_HSLINK_PRO_EXPANSION_H
#define HPM5300EVKLITE_DAP_HSLINK_PRO_EXPANSION_H

typedef enum {
    USER_ADC_VREF_CHANNEL = 11,
    USER_ADC_TVCC_CHANNEL = 1,
} USER_ADC_CHANNEL_t;

#ifdef __cplusplus
extern "C"
{
#endif

extern volatile bool VREF_ENABLE;

typedef struct  NeoPixel NeoPixel;
extern NeoPixel *neopixel;

/**
 * @brief 外部扩展初始化
 */
void HSP_Init(void);

/**
 * @brief 外部扩展循环，放入主循环中
 */
void HSP_Loop(void);

void HSP_Reboot(void);

void HSP_EnterHSLinkBootloader(void);

void HSP_EntrySysBootloader(void);

void Power_Turn(bool on);

void Port_Turn(bool on);

#ifdef __cplusplus
}
#endif

#endif //HPM5300EVKLITE_DAP_HSLINK_PRO_EXPANSION_H
