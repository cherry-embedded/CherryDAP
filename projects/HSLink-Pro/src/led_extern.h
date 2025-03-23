//
// Created by HalfSweet on 25-3-23.
//

#ifndef HSLINK_PRO_LED_EXTERN_H
#define HSLINK_PRO_LED_EXTERN_H

#include <stdint.h>

#ifdef __cplusplus
#include "LED.h"
extern LED led;
extern "C"
{
#endif

void LED_SetConnectMode(bool on);

void LED_SetRunningMode(bool on);

void LED_SetBrightness(uint8_t brightness);

void LED_SetBoost(bool boost);

void LED_SetEnable(bool enable);

#ifdef __cplusplus
}
#endif

#endif //HSLINK_PRO_LED_EXTERN_H
