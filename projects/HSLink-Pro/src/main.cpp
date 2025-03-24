#include "HSLink_Pro_expansion.h"
#include "LED.h"
#include "board.h"
#include "dap_main.h"
#include "setting.h"
#include "usb2uart.h"
#include "usb_configuration.h"
#include <elog.h>
#include <hid_comm.h>
#include <hpm_dma_mgr.h>
#include <hpm_ewdg_drv.h>
#include <hpm_gpio_drv.h>
#include <hpm_gpiom_drv.h>
#include <hpm_romapi.h>

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

    HSP_Init();
    intc_set_irq_priority(CONFIG_HPM_USBD_IRQn, 5);
    uartx_preinit();
    USB_Configuration();

    led.SetNeoPixel(neopixel);

    bl_setting.app_version.major = CONFIG_BUILD_VERSION_MAJOR;
    bl_setting.app_version.minor = CONFIG_BUILD_VERSION_MINOR;
    bl_setting.app_version.patch = CONFIG_BUILD_VERSION_PATCH;

    bl_setting.fail_cnt = 0;

    while (true) {
        ewdg_refresh(HPM_EWDG0);
        chry_dap_handle();
        chry_dap_usb2uart_handle();
        usb2uart_handler();
        HSP_Loop();
#ifdef CONFIG_USE_HID_CONFIG
        HID_Handle();
#endif
        led.Handle();
    }
}
