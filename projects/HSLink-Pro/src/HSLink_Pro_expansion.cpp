/*
 * Copyright (c) 2024 HalfSweet
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include "hpm_common.h"
#include <hpm_gpiom_drv.h>
#include <hpm_l1c_drv.h>
#include <hpm_romapi.h>
#include <neopixel.h>
#include <multi_button.h>
#include <hpm_ewdg_drv.h>
#include <hpm_spi.h>
#include "HSLink_Pro_expansion.h"
#include "board.h"
#include "hpm_gptmr_drv.h"
#include "hpm_gpio_drv.h"
#include "hpm_adc16_drv.h"
#include "setting.h"

volatile bool VREF_ENABLE = false;

const double ADC_REF = 3.3;

GPTMR_Type *const USER_PWM = HPM_GPTMR0;
const clock_name_t USER_PWM_CLK = clock_gptmr0;
const uint8_t PWM_CHANNEL = 2;

ADC16_Type *const USER_ADC = HPM_ADC0;

const uint32_t DEFAULT_PWM_FREQ = 100000;
const uint8_t DEFAULT_PWM_DUTY = 50;

const uint8_t DEFAULT_ADC_RUN_MODE = adc16_conv_mode_oneshot;
const uint8_t DEFAULT_ADC_CYCLE = 20;

static uint32_t pwm_current_reload;

static const uint32_t CONFIG_P_EN = IOC_PAD_PA31;

static void set_pwm_waveform_edge_aligned_frequency(uint32_t freq) {
    gptmr_channel_config_t config;
    uint32_t gptmr_freq;

    gptmr_channel_get_default_config(USER_PWM, &config);
    gptmr_freq = clock_get_frequency(USER_PWM_CLK);
    pwm_current_reload = gptmr_freq / freq;
    config.reload = pwm_current_reload;
    config.cmp_initial_polarity_high = true;
    gptmr_stop_counter(USER_PWM, PWM_CHANNEL);
    gptmr_channel_config(USER_PWM, PWM_CHANNEL, &config, false);
    gptmr_channel_reset_count(USER_PWM, PWM_CHANNEL);
    gptmr_start_counter(USER_PWM, PWM_CHANNEL);
}

static void set_pwm_waveform_edge_aligned_duty(uint8_t duty) {
    uint32_t cmp;
    if (duty > 100) {
        duty = 100;
    }
    cmp = ((pwm_current_reload * duty) / 100) + 1;
    gptmr_update_cmp(USER_PWM, PWM_CHANNEL, 0, cmp);
}

static void Power_Enable_Init(void) {
    HPM_IOC->PAD[CONFIG_P_EN].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);

    gpiom_set_pin_controller(HPM_GPIOM, GPIO_GET_PORT_INDEX(CONFIG_P_EN), GPIO_GET_PIN_INDEX(CONFIG_P_EN),
                             gpiom_soc_gpio0);
    gpio_set_pin_output(HPM_GPIO0, GPIO_GET_PORT_INDEX(CONFIG_P_EN), GPIO_GET_PIN_INDEX(CONFIG_P_EN));
    gpio_write_pin(HPM_GPIO0, GPIO_GET_PORT_INDEX(CONFIG_P_EN), GPIO_GET_PIN_INDEX(CONFIG_P_EN),
                   HSLink_Setting.power.power_on);
}

void Power_Turn_On(void) {
    gpio_write_pin(HPM_GPIO0, GPIO_GET_PORT_INDEX(CONFIG_P_EN), GPIO_GET_PIN_INDEX(CONFIG_P_EN), 1);
}

void Power_Turn_Off(void) {
    gpio_write_pin(HPM_GPIO0, GPIO_GET_PORT_INDEX(CONFIG_P_EN), GPIO_GET_PIN_INDEX(CONFIG_P_EN), 0);
}

static void Power_PWM_Init(void) {
    // PA10 100k PWM，占空比50%
    HPM_IOC->PAD[IOC_PAD_PA10].FUNC_CTL = IOC_PA10_FUNC_CTL_GPTMR0_COMP_2;
    set_pwm_waveform_edge_aligned_frequency(DEFAULT_PWM_FREQ);
    set_pwm_waveform_edge_aligned_duty(DEFAULT_PWM_DUTY);
}

static void Power_Set_TVCC_Voltage(double voltage) {
    // voltage = -DAC + (1974/395)
    // DAC = -voltage + (1974/395)

    // PWM DAC需要输出的电压
    double dac = -voltage + (1974.0 / 395.0);
    if (dac < 0) {
        dac = 0;
    } else if (dac > 3.3) {
        dac = 3.3;
    }

    // 计算PWM占空比
    uint8_t duty = (uint8_t) (dac / 3.3 * 100);
    set_pwm_waveform_edge_aligned_duty(duty);
}

static void ADC_Init() {
    /* Configure the ADC clock from AHB (@200MHz by default)*/
    clock_set_adc_source(clock_adc0, clk_adc_src_ahb0);

    adc16_config_t cfg;

    /* initialize an ADC instance */
    adc16_get_default_config(&cfg);

    cfg.res = adc16_res_16_bits;
    cfg.conv_mode = DEFAULT_ADC_RUN_MODE;
    cfg.adc_clk_div = adc16_clock_divider_4;
    cfg.sel_sync_ahb = (clk_adc_src_ahb0 == clock_get_source(BOARD_APP_ADC16_CLK_NAME)) ? true : false;

    if (cfg.conv_mode == adc16_conv_mode_sequence ||
        cfg.conv_mode == adc16_conv_mode_preemption) {
        cfg.adc_ahb_en = true;
    }

    /* adc16 initialization */
    if (adc16_init(USER_ADC, &cfg) == status_success) {
        /* enable irq */
        //        intc_m_enable_irq_with_priority(BOARD_APP_ADC16_IRQn, 1);
    }
}

static void ADC_Channel_Init(USER_ADC_CHANNEL_t channel) {
    adc16_channel_config_t ch_cfg;

    /* get a default channel config */
    adc16_get_channel_default_config(&ch_cfg);

    /* initialize an ADC channel */
    ch_cfg.ch = channel;
    ch_cfg.sample_cycle = DEFAULT_ADC_CYCLE;

    adc16_init_channel(USER_ADC, &ch_cfg);

    adc16_set_nonblocking_read(USER_ADC);

#if defined(ADC_SOC_BUSMODE_ENABLE_CTRL_SUPPORT) && ADC_SOC_BUSMODE_ENABLE_CTRL_SUPPORT
    /* enable oneshot mode */
    adc16_enable_oneshot_mode(USER_ADC);
#endif
}

static uint16_t Get_ADC_Value(USER_ADC_CHANNEL_t channel) {
    ADC_Channel_Init(channel);

    uint16_t result = 0;

    if (adc16_get_oneshot_result(USER_ADC, channel, &result) == status_success) {
        if (adc16_is_nonblocking_mode(USER_ADC)) {
            adc16_get_oneshot_result(USER_ADC, channel, &result);
        }
    }

    return result;
}

ATTR_ALWAYS_INLINE
static inline void VREF_Init(void) {
    // VREF 的输入是 PB08
    HPM_IOC->PAD[IOC_PAD_PB08].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;
}

ATTR_ALWAYS_INLINE
static inline void TVCC_Init(void) {
    // TVCC 的输入是 PB09
    HPM_IOC->PAD[IOC_PAD_PB09].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;
}

ATTR_ALWAYS_INLINE
static inline double Get_VREF_Voltage(void) {
    return (double) Get_ADC_Value(USER_ADC_VREF_CHANNEL) * ADC_REF / 65535 * 2;
}

ATTR_ALWAYS_INLINE
static double inline Get_TVCC_Voltage(void) {
    return (double) Get_ADC_Value(USER_ADC_TVCC_CHANNEL) * ADC_REF / 65535 * 2;
}

ATTR_ALWAYS_INLINE
static inline void BOOT_Init(void) {
    // PA03
    HPM_IOC->PAD[IOC_PAD_PA03].FUNC_CTL = IOC_PA03_FUNC_CTL_GPIO_A_03;

    gpiom_set_pin_controller(HPM_GPIOM, GPIOM_ASSIGN_GPIOA, 3, gpiom_soc_gpio0);
    gpio_set_pin_input(HPM_GPIO0, GPIO_OE_GPIOA, 3);
    gpio_disable_pin_interrupt(HPM_GPIO0, GPIO_IE_GPIOA, 3);
}

ATTR_ALWAYS_INLINE
static inline void Port_Enable_Init(void) {
    // PA04
    HPM_IOC->PAD[IOC_PAD_PA04].FUNC_CTL = IOC_PA04_FUNC_CTL_GPIO_A_04;

    gpiom_set_pin_controller(HPM_GPIOM, GPIOM_ASSIGN_GPIOA, 4, gpiom_soc_gpio0);
    gpio_set_pin_output(HPM_GPIO0, GPIO_OE_GPIOA, 4);

    gpio_write_pin(HPM_GPIO0, GPIO_OE_GPIOA, 4, HSLink_Setting.power.port_on);
}

ATTR_ALWAYS_INLINE
static inline void Port_Turn_Enable(void) {
    gpio_write_pin(HPM_GPIO0, GPIO_OE_GPIOA, 4, 1);
}

ATTR_ALWAYS_INLINE
static inline void Port_Turn_Disable(void) {
    gpio_write_pin(HPM_GPIO0, GPIO_OE_GPIOA, 4, 0);
}

ATTR_ALWAYS_INLINE
static inline void Power_Trun(bool on) {
    gpio_write_pin(HPM_GPIO0, GPIO_GET_PORT_INDEX(CONFIG_P_EN), GPIO_GET_PIN_INDEX(CONFIG_P_EN), on);
}

ATTR_ALWAYS_INLINE
static inline void Port_Turn(bool on) {
    gpio_write_pin(HPM_GPIO0, GPIO_OE_GPIOA, 4, on);
}

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

NeoPixel *neopixel = nullptr;

static void WS2812_Init(void) {
    if (CheckHardwareVersion(0, 0, 0) or
        CheckHardwareVersion(1, 0, 0xFF) or
        CheckHardwareVersion(1, 1, 0xFF)
            ) {
        auto _neopixel = new NeoPixel_GPIO_Polling{1};
        NeoPixel_GPIO_Polling::interface_config_t config = {
                .init = __WS2812_Config_Init,
                .set_level = __WS2812_Config_SetLevel,
                .lock = __WS2812_Config_Lock,
                .unlock = __WS2812_Config_Unlock,
                .high_nop_cnt = 50,
                .low_nop_cnt = 15,
                .user_data = nullptr,
        };
        _neopixel->SetInterfaceConfig(&config);
        neopixel = reinterpret_cast<NeoPixel *>(_neopixel);
    } else if (CheckHardwareVersion(1, 2, 0xFF)) {
        auto _neopixel = new NeoPixel_SPI_Polling{1};
        NeoPixel_SPI_Polling::interface_config_t config = {
                .init = [](void *) {
                    HPM_IOC->PAD[IOC_PAD_PA07].FUNC_CTL = IOC_PA07_FUNC_CTL_SPI0_MOSI;

                    spi_initialize_config_t init_config;
                    hpm_spi_get_default_init_config(&init_config);
                    init_config.direction = msb_first;
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
                .trans = [](uint8_t *data, uint32_t len, void *user_data) {
                    hpm_spi_transmit_blocking(HPM_SPI0, data, len, 0xFFFF);
                },
                .user_data = nullptr
        };
        _neopixel->SetInterfaceConfig(&config);
        neopixel = reinterpret_cast<NeoPixel *>(_neopixel);
    }
}

#ifdef WS2812_TEST
extern "C" void HSP_WS2812_SetColor(uint8_t r, uint8_t g, uint8_t b) {
    if (!neopixel) {
        return;
    }
    neopixel->SetPixel(0, r, g, b);
    neopixel->Flush();
}

extern "C" void HSP_WS2812_SetRed(uint8_t r) {
    if (!neopixel) {
        return;
    }
    neopixel->ModifyPixel(0, NeoPixel::color_type_t::COLOR_R, r);
    neopixel->Flush();
}

extern "C" void HSP_WS2812_SetGreen(uint8_t g) {
    if (!neopixel) {
        return;
    }
    neopixel->ModifyPixel(0, NeoPixel::color_type_t::COLOR_G, g);
    neopixel->Flush();
}

extern "C" void HSP_WS2812_SetBlue(uint8_t b) {
    if (!neopixel) {
        return;
    }
    neopixel->ModifyPixel(0, NeoPixel::color_type_t::COLOR_B, b);
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
#endif

static void Button_Init() {
    static Button btn;
    button_init(&btn, [](uint8_t) {
        return gpio_read_pin(BOARD_BTN_GPIO_CTRL, BOARD_BTN_GPIO_INDEX, BOARD_BTN_GPIO_PIN);
    }, BOARD_BTN_PRESSED_VALUE, 0);
    button_attach(&btn, SINGLE_CLICK, [](void *) {
        printf("single click, send reset to target\r\n");
        // TODO
    });
    button_attach(&btn, DOUBLE_CLICK, [](void *) {
        printf("double click, enter system Bootloader\n");
        Power_Turn_Off();
        HSP_EntrySysBootloader();
    });
    button_attach(&btn, LONG_PRESS_START, [](void *) {
        printf("long press, enter Bootloader\n");
        Power_Turn_Off();
        HSP_EnterHSLinkBootloader();
    });
    button_start(&btn);
}

extern "C" void HSP_Init(void) {
    // 初始化电源部分
    Power_Enable_Init();
    Port_Enable_Init();
    Power_PWM_Init();
    // 初始化ADC部分
    ADC_Init();
    VREF_Init();
    TVCC_Init();
    BOOT_Init();
    WS2812_Init();
    Button_Init();

#ifdef WS2812_TEST
    printf("blue\r\n");
    HSP_WS2812_SetBlue(100);
    board_delay_ms(1000);
    HSP_WS2812_SetBlue(0);
    ewdg_refresh(HPM_EWDG0);

    printf("green\r\n");
    HSP_WS2812_SetGreen(100);
    board_delay_ms(1000);
    HSP_WS2812_SetGreen(0);
    ewdg_refresh(HPM_EWDG0);

    printf("red\r\n");
    HSP_WS2812_SetRed(100);
    board_delay_ms(1000);
    HSP_WS2812_SetRed(0);
    ewdg_refresh(HPM_EWDG0);

    for (auto i = 0; i <1000; i++) {
        WS2812_ShowRainbow();
        board_delay_ms(10);
        ewdg_refresh(HPM_EWDG0);
    }
#endif
}

extern "C" void HSP_Loop(void) {
    // 检测VREF电压
    double vref = Get_VREF_Voltage();

    if (vref > 1.6) {
        Power_Set_TVCC_Voltage(vref);
        Power_Turn_On();
        Port_Turn_Enable();
        VREF_ENABLE = true;
    } else {
        Power_Trun(HSLink_Setting.power.power_on);
        Power_Set_TVCC_Voltage(HSLink_Setting.power.vref); // TVCC恢复默认设置
        Port_Turn(HSLink_Setting.power.port_on);
        VREF_ENABLE = false;
    }

    static uint32_t last_btn_chk_time = 0;
    if (millis() - last_btn_chk_time > TICKS_INTERVAL) {
        last_btn_chk_time = millis();
        button_ticks();
    }
}

extern "C" void HSP_Reboot(void) {
    bl_setting.keep_bootloader = 0;
    neopixel->SetPixel(0, 0, 0, 0);
    neopixel->Flush();
    disable_global_irq(CSR_MSTATUS_MIE_MASK);
    //disable_global_irq(CSR_MSTATUS_SIE_MASK);
    disable_global_irq(CSR_MSTATUS_UIE_MASK);
    l1c_dc_invalidate_all();
    l1c_dc_disable();
    fencei();

    api_boot_arg_t boot_arg = {
            .index = 0,
            .peripheral = API_BOOT_PERIPH_AUTO,
            .src = API_BOOT_SRC_PRIMARY,
            .tag = API_BOOT_TAG,
    };

    ROM_API_TABLE_ROOT->run_bootloader(&boot_arg);
}

extern "C" void HSP_EnterHSLinkBootloader(void) {
    bl_setting.keep_bootloader = 1;
    neopixel->SetPixel(0, 0, 0, 0);
    neopixel->Flush();
    disable_global_irq(CSR_MSTATUS_MIE_MASK);
    //disable_global_irq(CSR_MSTATUS_SIE_MASK);
    disable_global_irq(CSR_MSTATUS_UIE_MASK);
    l1c_dc_invalidate_all();
    l1c_dc_disable();
    fencei();

    api_boot_arg_t boot_arg = {
            .index = 0,
            .peripheral = API_BOOT_PERIPH_AUTO,
            .src = API_BOOT_SRC_PRIMARY,
            .tag = API_BOOT_TAG,
    };

    ROM_API_TABLE_ROOT->run_bootloader(&boot_arg);
}

extern "C" void HSP_EntrySysBootloader(void) {
    neopixel->SetPixel(0, 0, 0, 0);
    neopixel->Flush();

    disable_global_irq(CSR_MSTATUS_MIE_MASK);
    //disable_global_irq(CSR_MSTATUS_SIE_MASK);
    disable_global_irq(CSR_MSTATUS_UIE_MASK);
    l1c_dc_invalidate_all();
    l1c_dc_disable();
    fencei();

    api_boot_arg_t boot_arg = {
            .index = 0,
            .peripheral = API_BOOT_PERIPH_UART,
            .src = API_BOOT_SRC_ISP,
            .tag = API_BOOT_TAG,
    };

    ROM_API_TABLE_ROOT->run_bootloader(&boot_arg);
}
