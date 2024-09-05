#include "board.h"
#include "dap_main.h"
#include "bflb_gpio.h"
#include "usb2uart.h"

int main(void)
{
    board_init();

    uartx_preinit();

    chry_dap_init(0, 0x20072000);
    while (1) {
        chry_dap_handle();
        chry_dap_usb2uart_handle();
    }
}
