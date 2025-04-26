//
// Created by yekai on 2025/3/11.
//

#include "ws2812.h"
#include "neopixel.h"
#include "usb_config.h"
#include <board.h>
#include <hpm_gpio_drv.h>
#include <hpm_spi.h>

NeoPixel *neopixel = nullptr;
ATTR_RAMFUNC
static void __WS2812_Config_Init(void *user_data) {
    HPM_IOC->PAD[IOC_PAD_PA02].FUNC_CTL = IOC_PA02_FUNC_CTL_GPIO_A_02;
    //    HPM_IOC->PAD[IOC_PAD_PA07].FUNC_CTL = IOC_PA07_FUNC_CTL_GPIO_A_07;
    gpio_set_pin_output(BOARD_LED_GPIO_CTRL, BOARD_LED_GPIO_INDEX,
                        BOARD_LED_GPIO_PIN);
    gpio_write_pin(BOARD_LED_GPIO_CTRL, BOARD_LED_GPIO_INDEX,
                   BOARD_LED_GPIO_PIN, 0);
}
ATTR_RAMFUNC
static void __WS2812_Config_SetLevel(uint8_t level, void *user_data) {
    gpio_write_pin(BOARD_LED_GPIO_CTRL, BOARD_LED_GPIO_INDEX,
                   BOARD_LED_GPIO_PIN, level);
    __asm volatile("fence io, io");
}
ATTR_RAMFUNC
static void __WS2812_Config_Lock(void *user_data) {
    disable_global_irq(CSR_MSTATUS_MIE_MASK);
}

ATTR_RAMFUNC
static void __WS2812_Config_Unlock(void *user_data) {
    enable_global_irq(CSR_MSTATUS_MIE_MASK);
}

extern "C" void WS2812_Init() {
    if (neopixel) {
        delete neopixel;
    }
    if (CheckHardwareVersion(0, 0, 0) or
        CheckHardwareVersion(1, 0, 0xFF) or
        CheckHardwareVersion(1, 1, 0xFF)) {
        auto neopixel_gpio = new NeoPixel_GPIO_Polling{1};
        NeoPixel_GPIO_Polling::interface_config_t config = {
                .init = __WS2812_Config_Init,
                .set_level = __WS2812_Config_SetLevel,
                .lock = __WS2812_Config_Lock,
                .unlock = __WS2812_Config_Unlock,
                .high_nop_cnt = 65,
                .low_nop_cnt = 18,
                .user_data = nullptr,
        };
        neopixel_gpio->SetInterfaceConfig(&config);
        neopixel = reinterpret_cast<NeoPixel *>(neopixel_gpio);
    } else if (CheckHardwareVersion(1, 2, 0xFF) or CheckHardwareVersion(1,3,0xFF)) {
        auto _neopixel = new NeoPixel_SPI_Polling{1};
        NeoPixel_SPI_Polling::interface_config_t config = {
                .init = [](void *) {
                    HPM_IOC->PAD[IOC_PAD_PA07].FUNC_CTL = IOC_PA07_FUNC_CTL_SPI0_MOSI;

                    spi_initialize_config_t init_config;
                    hpm_spi_get_default_init_config(&init_config);
                    init_config.direction = spi_msb_first;
                    init_config.mode = spi_master_mode;
                    init_config.clk_phase = spi_sclk_sampling_odd_clk_edges;
                    init_config.clk_polarity = spi_sclk_low_idle;
                    init_config.data_len = 8;
                    /* step.1  initialize spi */
                    if (hpm_spi_initialize(HPM_SPI0, &init_config) != status_success) {
                        printf("SPI init failed!\r\n");
                        while (1) {
                        }
                    }

                    /* step.2  set spi sclk frequency for master */
                    if (hpm_spi_set_sclk_frequency(HPM_SPI0, 8 * 1000 * 1000) != status_success) {
                        printf("hpm_spi_set_sclk_frequency fail\n");
                        while (1) {
                        }
                    }
                },
                .trans = [](uint8_t *data, uint32_t len, void *user_data) { hpm_spi_transmit_blocking(HPM_SPI0, data, len, 0xFFFF); },
                .user_data = nullptr};
        _neopixel->SetInterfaceConfig(&config);
        neopixel = reinterpret_cast<NeoPixel *>(_neopixel);
    }
}
extern "C" void WS2812_SetColor(uint32_t color) {
    if (!neopixel)
        return;
    neopixel->SetPixel(0, color);
    neopixel->Flush();
}

extern "C" void WS2812_ShowFadeOn() {
    if (!neopixel)
        return;

    static uint8_t j = 0;
    j++;
    //    printf("FadeOn: %d\n", j);
    neopixel->SetPixel(0, j, j, j);
    neopixel->Flush();
}

extern "C" void WS2812_ShowRainbow() {
    if (!neopixel)
        return;

    static int j = 0;
    j++;
    uint8_t r, g, b;
    uint8_t pos = j & 255;
    if (pos < 85) {
        r = pos * 3;
        g = 255 - pos * 3;
        b = 0;
    } else if (pos < 170) {
        pos -= 85;
        r = 255 - pos * 3;
        g = 0;
        b = pos * 3;
    } else {
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

extern "C" void WS2812_TurnOff() {
    if (!neopixel)
        return;
    neopixel->SetPixel(0, 0, 0, 0);
    neopixel->Flush();
}