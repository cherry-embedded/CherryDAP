//
// Created by HalfSweet on 25-3-23.
//

#include "led_extern.h"
#include <stdint.h>
#include "HSLink_Pro_expansion.h"
#include "LED.h"

LED led;

extern "C" {
void LED_SetConnectMode(bool on)
{
    if (on) {
        led.SetMode(LEDMode::CONNECTED);
    } else {
        led.SetMode(LEDMode::IDLE);
    }
}

void LED_SetRunningMode(bool on)
{
    if (on) {
        led.SetMode(LEDMode::RUNNING);
    } else {
        led.SetMode(LEDMode::CONNECTED);
    }
}

void LED_SetBrightness(uint8_t brightness)
{
    led.SetBrightness(brightness);
}

void LED_SetBoost(bool boost)
{
    led.SetBoost(boost);
}

void LED_SetEnable(bool enable)
{
    led.SetEnable(enable);
}

}
