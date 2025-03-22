#include "BL_Setting_Common.h"
#include "bootuf2.h"
#include "third_party_components/port/neopixel/ws2812.h"
#include "usb_config.h"
#include <board.h>
#include <hpm_dma_mgr.h>
#include <hpm_ewdg_drv.h>
#include <hpm_gpio_drv.h>
#include <hpm_gpiom_drv.h>
#include <hpm_l1c_drv.h>
#include <hpm_ppor_drv.h>
#include <multi_button.h>
#include <usb_log.h>

#if defined(CONFIG_WS2812) && CONFIG_WS2812 == 1
#include <WS2812.h>
#endif

ATTR_PLACE_AT(".bl_setting")
static BL_Setting_t bl_setting;

static const uint32_t CONFIG_P_EN = IOC_PAD_PA31;
static const uint32_t CONFIG_Port_EN = IOC_PAD_PA04;

static void jump_app(void)
{
    fencei();
    l1c_dc_disable();
    __asm("la a0, %0" ::"i"(CONFIG_BOOTUF2_APP_START + 4));
    __asm("jr a0");
}

static void bootloader_button_init(void)
{
    // BOOT1(PA03)
    HPM_IOC->PAD[IOC_PAD_PA03].FUNC_CTL = IOC_PA03_FUNC_CTL_GPIO_A_03;

    gpiom_set_pin_controller(HPM_GPIOM, GPIOM_ASSIGN_GPIOA, 3, gpiom_soc_gpio0);
    gpio_set_pin_input(HPM_GPIO0, GPIO_OE_GPIOA, 3);
    gpio_disable_pin_interrupt(HPM_GPIO0, GPIO_IE_GPIOA, 3);
}

/**
 * @brief 验证APP区是否合法
 * @return true: APP区合法 false: APP区不合法
 */
static bool app_valid(void)
{
    if (((volatile uint32_t const *)CONFIG_BOOTUF2_APP_START)[0] != BOARD_UF2_SIGNATURE)
    {
        return false;
    }
    return true;
}

static void IOInit(void)
{
    // 将输出全部设置为高阻态

    // PowerEN
    HPM_IOC->PAD[CONFIG_P_EN].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);

    gpiom_set_pin_controller(HPM_GPIOM, GPIO_GET_PORT_INDEX(CONFIG_P_EN), GPIO_GET_PIN_INDEX(CONFIG_P_EN), gpiom_soc_gpio0);
    gpio_set_pin_output(HPM_GPIO0, GPIO_GET_PORT_INDEX(CONFIG_P_EN), GPIO_GET_PIN_INDEX(CONFIG_P_EN));
    gpio_write_pin(HPM_GPIO0, GPIO_GET_PORT_INDEX(CONFIG_P_EN), GPIO_GET_PIN_INDEX(CONFIG_P_EN), 0);

    // PortEN
    HPM_IOC->PAD[CONFIG_Port_EN].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);

    gpiom_set_pin_controller(HPM_GPIOM, GPIO_GET_PORT_INDEX(CONFIG_Port_EN), GPIO_GET_PIN_INDEX(CONFIG_Port_EN), gpiom_soc_gpio0);
    gpio_set_pin_output(HPM_GPIO0, GPIO_GET_PORT_INDEX(CONFIG_Port_EN), GPIO_GET_PIN_INDEX(CONFIG_Port_EN));
    gpio_write_pin(HPM_GPIO0, GPIO_GET_PORT_INDEX(CONFIG_Port_EN), GPIO_GET_PIN_INDEX(CONFIG_Port_EN), 0);
}

static void EWDG_Init()
{
    clock_add_to_group(clock_watchdog0, 0);
    ewdg_config_t config;
    ewdg_get_default_config(HPM_EWDG0, &config);

    config.enable_watchdog = true;
    config.int_rst_config.enable_timeout_reset = true;
    config.ctrl_config.use_lowlevel_timeout = false;
    config.ctrl_config.cnt_clk_sel = ewdg_cnt_clk_src_ext_osc_clk;

    /* Set the EWDG reset timeout to 5 second */
    config.cnt_src_freq = 32768;
    config.ctrl_config.timeout_reset_us = 5 * 1000 * 1000;

    /* Initialize the WDG */
    hpm_stat_t status = ewdg_init(HPM_EWDG0, &config);
    if (status != status_success)
    {
        printf(" EWDG initialization failed, error_code=%d\n", status);
    }
}

extern const char *file_INFO;
extern void msc_bootuf2_init(uint8_t busid, uint32_t reg_base);
extern void bootuf2_SetReason(const char *reason);

static bool button_is_pressed = false;

static uint8_t read_button_pin(uint8_t button_id)
{
    return gpio_read_pin(BOARD_BTN_GPIO_CTRL, BOARD_BTN_GPIO_INDEX, BOARD_BTN_GPIO_PIN);
}

static void button_press_cb(void *btn)
{
    printf("Button pressed\n");
    button_is_pressed = true;
}

int main(void)
{
    board_init();
    EWDG_Init();
    IOInit();
    dma_mgr_init();
    board_init_usb(HPM_USB0);
    bootloader_button_init();
    WS2812_Init();
    WS2812_TurnOff();

    if (bl_setting.magic != BL_SETTING_MAGIC)
    {
        memset(&bl_setting, 0x00, sizeof(bl_setting));
        bl_setting.magic = BL_SETTING_MAGIC;
    }

    bl_setting.bl_version.major = CONFIG_BUILD_VERSION_MAJOR;
    bl_setting.bl_version.minor = CONFIG_BUILD_VERSION_MINOR;
    bl_setting.bl_version.patch = CONFIG_BUILD_VERSION_PATCH;

    bl_setting.fail_cnt += 1;

    //        bl_setting.keep_bootloader = true;

    // 检测是否由APP触发
    if (bl_setting.keep_bootloader)
    {
        bootuf2_SetReason("APP_REQ_KEEP_BL");
        printf("APP_REQ_KEEP_BL\n");
        goto __entry_bl;
    }

    // 检测APP是否合法
    if (!app_valid())
    {
        bootuf2_SetReason("APP_INVALID");
        printf("APP_INVALID\n");
        goto __entry_bl;
    }

    // 检测是否启动失败次数超过阈发值
    if (bl_setting.fail_cnt > 5)
    {
        bl_setting.fail_cnt = 0;
        bootuf2_SetReason("FAIL_CNT_OVER_THRESHOLD");
        printf("FAIL_CNT_OVER_THRESHOLD\n");
        goto __entry_bl;
    }

    Button btn;
    button_init(&btn, read_button_pin, BOARD_BTN_PRESSED_VALUE, 0);
    button_attach(&btn, PRESS_DOWN, button_press_cb);
    button_start(&btn);

    // blink blue when booting, press button to enter bootloader
    bool led_sta = false;
    for (int i = 0; i < 2000 / 5; i++)
    {
        if (i % 100 == 0)
        {
            if (led_sta)
            {
                WS2812_SetColor(0);
            }
            else
            {
                WS2812_SetColor(0xFF / 8);
            }
            led_sta = !led_sta;
        }
        button_ticks();
        board_delay_ms(5);
    }

    // 检测2s内是否有按键按下
    if (button_is_pressed)
    {
        bootuf2_SetReason("BUTTON_PRESSED_IN_2S");
        printf("BUTTON_PRESSED_IN_2S\n");
        goto __entry_bl;
    }

    goto __entry_app;

__entry_bl:
    intc_set_irq_priority(CONFIG_HPM_USBD_IRQn, 2);
    printf("HSLink Pro UF2 Bootloader\n");

    msc_bootuf2_init(0, CONFIG_HPM_USBD_BASE);

    while (1)
    {
        if (bootuf2_is_write_done())
        {
            USB_LOG_INFO("Update success! Jump to application.\n");
            bl_setting.keep_bootloader = false;
            ppor_reset_mask_set_source_enable(HPM_PPOR, ppor_reset_software);
            ppor_sw_reset(HPM_PPOR, 1000);
        }
        WS2812_ShowRainbow();
//        WS2812_ShowFadeOn();
        board_delay_ms(10);
        ewdg_refresh(HPM_EWDG0);
    }
    return 0;

__entry_app:
    WS2812_TurnOff();
    USB_LOG_INFO("Jump to application @0x%x(0x%x)\r\n", CONFIG_BOOTUF2_APP_START, *(volatile uint32_t *)CONFIG_BOOTUF2_APP_START);
    jump_app();
    while (1)
        ;
}
