#include <hpm_romapi.h>
#include <hpm_dma_mgr.h>
#include <hpm_gpio_drv.h>
#include <hpm_gpiom_drv.h>
#include "board.h"
#include "dap_main.h"
#include "WS2812.h"
#include "projects/HSLink-Pro/common/HSLink_Pro_expansion.h"
#include "usb2uart.h"
#include "usb_configuration.h"

char serial_number[32];

static void serial_number_init(void)
{
#define OTP_CHIP_UUID_IDX_START (88U)
#define OTP_CHIP_UUID_IDX_END   (91U)
    uint32_t uuid_words[4];

    uint32_t word_idx = 0;
    for (uint32_t i = OTP_CHIP_UUID_IDX_START; i <= OTP_CHIP_UUID_IDX_END; i++) {
        uuid_words[word_idx++] = ROM_API_TABLE_ROOT->otp_driver_if->read_from_shadow(i);
    }

    char chip_id[32];
    sprintf(chip_id, "%08X%08X%08X%08X", uuid_words[0], uuid_words[1], uuid_words[2], uuid_words[3]);
    memcpy(serial_number, chip_id, 32);

    printf("Serial number: %s\n", serial_number);
}

ATTR_ALWAYS_INLINE
static inline void SWDIO_DIR_Init(void)
{
    HPM_IOC->PAD[SWDIO_DIR].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);

    gpiom_set_pin_controller(HPM_GPIOM, GPIO_GET_PORT_INDEX(SWDIO_DIR), GPIO_GET_PIN_INDEX(SWDIO_DIR), gpiom_soc_gpio0);
    gpio_set_pin_output(HPM_GPIO0, GPIO_GET_PORT_INDEX(SWDIO_DIR), GPIO_GET_PIN_INDEX(SWDIO_DIR));
    gpio_write_pin(HPM_GPIO0, GPIO_GET_PORT_INDEX(SWDIO_DIR), GPIO_GET_PIN_INDEX(SWDIO_DIR), 1);
}

int main(void)
{
    board_init();
    serial_number_init();
    board_init_usb_pins();
    dma_mgr_init();

    SWDIO_DIR_Init();

    printf("version: " CONFIG_BUILD_VERSION "\n");
    extern char *string_descriptors[];
    memcpy(string_descriptors[3], serial_number, 32);

    HSP_Init();

    intc_set_irq_priority(CONFIG_HPM_USBD_IRQn, 5);
    uartx_preinit();

    USB_Configuration();

    while (1) {
        chry_dap_handle();
        chry_dap_usb2uart_handle();
        usb2uart_handler();
        HSP_Loop();
    }
}
