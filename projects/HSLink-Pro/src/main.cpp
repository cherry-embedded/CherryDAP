#include <hid_comm.h>
#include <hpm_romapi.h>
#include <hpm_dma_mgr.h>
#include <hpm_gpio_drv.h>
#include <hpm_gpiom_drv.h>
#include <hpm_ewdg_drv.h>
#include <neopixel.h>
#include "board.h"
#include "dap_main.h"
#include "HSLink_Pro_expansion.h"
#include "usb2uart.h"
#include "usb_configuration.h"
#include "setting.h"

static void serial_number_init(void) {
#define OTP_CHIP_UUID_IDX_START (88U)
#define OTP_CHIP_UUID_IDX_END   (91U)
    uint32_t uuid_words[4];

    uint32_t word_idx = 0;
    for (uint32_t i = OTP_CHIP_UUID_IDX_START; i <= OTP_CHIP_UUID_IDX_END; i++) {
        uuid_words[word_idx++] = ROM_API_TABLE_ROOT->otp_driver_if->read_from_shadow(i);
    }

    sprintf(serial_number, "%08X%08X%08X%08X", uuid_words[0], uuid_words[1], uuid_words[2], uuid_words[3]);
    printf("Serial number: %s\n", serial_number);
}

ATTR_ALWAYS_INLINE
static inline void SWDIO_DIR_Init(void) {
    HPM_IOC->PAD[SWDIO_DIR].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);

    gpiom_set_pin_controller(HPM_GPIOM, GPIO_GET_PORT_INDEX(SWDIO_DIR), GPIO_GET_PIN_INDEX(SWDIO_DIR), gpiom_soc_gpio0);
    gpio_set_pin_output(HPM_GPIO0, GPIO_GET_PORT_INDEX(SWDIO_DIR), GPIO_GET_PIN_INDEX(SWDIO_DIR));
    gpio_write_pin(HPM_GPIO0, GPIO_GET_PORT_INDEX(SWDIO_DIR), GPIO_GET_PIN_INDEX(SWDIO_DIR), 1);
}

static void EWDG_Init() {
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
    if (status != status_success) {
        printf(" EWDG initialization failed, error_code=%d\n", status);
    }
}

ATTR_RAMFUNC
static void __WS2812_Config_Init(void *user_data)
{
    HPM_IOC->PAD[IOC_PAD_PA02].FUNC_CTL = IOC_PA02_FUNC_CTL_GPIO_A_02;
    gpio_set_pin_output(BOARD_LED_GPIO_CTRL, BOARD_LED_GPIO_INDEX,
                        BOARD_LED_GPIO_PIN);
    gpio_write_pin(BOARD_LED_GPIO_CTRL, BOARD_LED_GPIO_INDEX,
                   BOARD_LED_GPIO_PIN, 0);
}
ATTR_RAMFUNC
static void __WS2812_Config_SetLevel(uint8_t level, void *user_data)
{
    gpio_write_pin(BOARD_LED_GPIO_CTRL, BOARD_LED_GPIO_INDEX,
                   BOARD_LED_GPIO_PIN, level);
    __asm volatile("fence io, io");
}
ATTR_RAMFUNC
static void __WS2812_Config_Lock(void *user_data)
{
    disable_global_irq(CSR_MSTATUS_MIE_MASK);
}

ATTR_RAMFUNC
static void __WS2812_Config_Unlock(void *user_data)
{
    enable_global_irq(CSR_MSTATUS_MIE_MASK);
}

extern "C" void WS2812_ShowRainbow(NeoPixel* neopixel)
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

[[noreturn]] // make compiler happy
int main() {
    board_init();
    EWDG_Init();
    board_delay_ms(500);
    serial_number_init();
    board_init_usb(HPM_USB0);
    dma_mgr_init();

    SWDIO_DIR_Init();

    Setting_Init();

    printf("version: " CONFIG_BUILD_VERSION "\n");

    HSP_Init();
    intc_set_irq_priority(CONFIG_HPM_USBD_IRQn, 5);
    uartx_preinit();
    USB_Configuration();

    NeoPixel_GPIO_Polling neopixel(1);
    NeoPixel_GPIO_Polling::interface_config_t config = {
            .init = __WS2812_Config_Init,
            .set_level = __WS2812_Config_SetLevel,
            .lock = __WS2812_Config_Lock,
            .unlock = __WS2812_Config_Unlock,
            .high_nop_cnt = 45,
            .low_nop_cnt = 15,
            .user_data = nullptr,
    };
    neopixel.SetInterfaceConfig(&config);

    bl_setting.fail_cnt = 0;

    while (1) {
        ewdg_refresh(HPM_EWDG0);
        chry_dap_handle();
        chry_dap_usb2uart_handle();
        usb2uart_handler();
        HSP_Loop();
#ifdef CONFIG_USE_HID_CONFIG
        HID_Handle();
#endif
        WS2812_ShowRainbow(&neopixel);
        board_delay_ms(10);
    }
}
