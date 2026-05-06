#include "dap_main.h"
#include "hpm_dma_mgr.h"
#include "hpm_gpio_drv.h"
#include "hpm_gpiom_drv.h"
#include "hpm_romapi.h"
#include "board.h"
#include "usb2uart.h"

static void serial_number_init(void)
{
#define OTP_CHIP_UUID_IDX_START (88U)
#define OTP_CHIP_UUID_IDX_END   (91U)
    uint32_t uuid_words[4];

    uint32_t word_idx = 0;
    for (uint32_t i = OTP_CHIP_UUID_IDX_START; i <= OTP_CHIP_UUID_IDX_END; i++) {
        uuid_words[word_idx++] = ROM_API_TABLE_ROOT->otp_driver_if->read_from_shadow(i);
    }

    sprintf(serial_number_dynamic, "%08X%08X%08X%08X", uuid_words[0], uuid_words[1], uuid_words[2], uuid_words[3]);
    printf("Serial number: %s\n", serial_number_dynamic);
}

static inline void SWDIO_DIR_Init(void)
{
    HPM_IOC->PAD[SWDIO_DIR].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);

    gpiom_set_pin_controller(HPM_GPIOM, GPIO_GET_PORT_INDEX(SWDIO_DIR), GPIO_GET_PIN_INDEX(SWDIO_DIR), PIN_GPIOM);
    gpio_set_pin_output(PIN_GPIO, GPIO_GET_PORT_INDEX(SWDIO_DIR), GPIO_GET_PIN_INDEX(SWDIO_DIR));
    gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(SWDIO_DIR), GPIO_GET_PIN_INDEX(SWDIO_DIR), 1);
}

HSLink_Setting_t HSLink_Setting;
HSLink_Lazy_t HSLink_Global;

int main(void)
{
    board_init();
    board_init_usb(HPM_USB0);
    intc_set_irq_priority(IRQn_USB0, 3);

    HSLink_Global.reset_level = 0;
    HSLink_Setting.reset = RESET_NRST;
    HSLink_Setting.jtag_20pin_compatible = false;
    HSLink_Setting.boost = false;
    HSLink_Setting.swd_port_mode = PORT_MODE_SPI;
    HSLink_Setting.jtag_port_mode = PORT_MODE_SPI;

    serial_number_init();
    dma_mgr_init();

    SWDIO_DIR_Init();

    uartx_preinit();
    chry_dap_init(0, HPM_USB0_BASE);

    while (1) {
        chry_dap_handle();
        chry_dap_usb2uart_handle();
        usb2uart_handler();
    }

    return 0;
}