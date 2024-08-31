#include <hpm_romapi.h>
#include <hpm_dma_mgr.h>
#include "board.h"
#include "dap_main.h"
#include "WS2812.h"
#include "projects/HSLink-Pro/common/HSLink_Pro_expansion.h"

extern void uartx_preinit(void);

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

int main(void)
{
    board_init();
    serial_number_init();
    board_init_led_pins();
    board_init_usb_pins();
    dma_mgr_init();

    printf("version: %s\n", DAP_FW_VER);
    extern char *string_descriptors[];
    string_descriptors[3] = serial_number;

    intc_set_irq_priority(CONFIG_HPM_USBD_IRQn, 2);
    uartx_preinit();

    WS2812_Init();
    HSP_Init();

    chry_dap_init(0, CONFIG_HPM_USBD_BASE);
    while (1) {
        chry_dap_handle();
        chry_dap_usb2uart_handle();
        HSP_Loop();
    }
}
