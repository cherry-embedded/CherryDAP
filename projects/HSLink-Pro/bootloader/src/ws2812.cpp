//
// Created by yekai on 2025/3/11.
//

#include "ws2812.h"
#include "neopixel.h"
#include "usb_config.h"
#include <board.h>
#include <hpm_gpio_drv.h>

NeoPixel *neopixel = nullptr;

extern "C" void WS2812_Init()
{
    if (neopixel)
    {
        delete neopixel;
    }
    auto neopixel_gpio = new NeoPixel_GPIO_Polling{1};
    NeoPixel_GPIO_Polling::interface_config_t config = {
        .init = [](void *user_data) {
            HPM_IOC->PAD[IOC_PAD_PA02].FUNC_CTL = IOC_PA02_FUNC_CTL_GPIO_A_02;
            gpio_set_pin_output(BOARD_LED_GPIO_CTRL, BOARD_LED_GPIO_INDEX,
                                BOARD_LED_GPIO_PIN);
            gpio_write_pin(BOARD_LED_GPIO_CTRL, BOARD_LED_GPIO_INDEX,
                           BOARD_LED_GPIO_PIN, 0);
        },
        .set_level = [](uint8_t level, void *user_data) {
            gpio_write_pin(BOARD_LED_GPIO_CTRL, BOARD_LED_GPIO_INDEX,
                           BOARD_LED_GPIO_PIN, level);
            __asm volatile("fence io, io");
        },
        .lock = [](void *user_data) {
            disable_global_irq(CSR_MSTATUS_MIE_MASK);
        },
        .unlock = [](void *user_data) {
            enable_global_irq(CSR_MSTATUS_MIE_MASK);
        },
        .high_nop_cnt = 45,
        .low_nop_cnt = 15,
        .user_data = nullptr,
    };
    neopixel_gpio->SetInterfaceConfig(&config);
    neopixel = reinterpret_cast<NeoPixel *>(neopixel_gpio);
}

extern "C" void WS2812_ShowFadeOn()
{
    if (!neopixel)
        return;

    static uint8_t j = 0;
    j++;
//    printf("FadeOn: %d\n", j);
    neopixel->SetPixel(0, j, j, j);
    neopixel->Flush();
}

extern "C" void WS2812_ShowRainbow()
{
    if (!neopixel)
        return;

    static int j = 0;
    j++;
    uint8_t r, g, b;
    uint8_t pos = j & 255;
    if (pos < 85)
    {
        r = pos * 3;
        g = 255 - pos * 3;
        b = 0;
    }
    else if (pos < 170)
    {
        pos -= 85;
        r = 255 - pos * 3;
        g = 0;
        b = pos * 3;
    }
    else
    {
        pos -= 170;
        r = 0;
        g = pos * 3;
        b = 255 - pos * 3;
    }
//    printf("Rainbow: %d, %d, %d\n", r, g, b);
    r = r / 16;
    g = g / 16;
    b = b / 16;
    neopixel->SetPixel(0, r, g, b);
    neopixel->Flush();
}

extern "C" void WS2812_TurnOff()
{
    if (!neopixel)
        return;
    neopixel->SetPixel(0, 0, 0, 0);
    neopixel->Flush();
}