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

/**
 * @brief 外部扩展初始化
 */
void HSP_Init(void);

/**
 * @brief 外部扩展循环，放入主循环中
 */
void HSP_Loop(void);

#endif //HPM5300EVKLITE_DAP_HSLINK_PRO_EXPANSION_H
