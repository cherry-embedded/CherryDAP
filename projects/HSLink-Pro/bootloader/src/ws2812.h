#pragma once

#include "stdint.h"

#ifdef __cplusplus
extern "C"
{
#endif

    void WS2812_Init();

    void WS2812_SetColor(uint32_t color);

    void WS2812_ShowFadeOn();

    void WS2812_ShowRainbow();

    void WS2812_TurnOff();

#ifdef __cplusplus
}
#endif