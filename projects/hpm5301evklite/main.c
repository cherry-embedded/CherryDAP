#include "board.h"
#include "dap_main.h"
#include "USER_Expansion.h"

extern void uartx_preinit(void);

int main(void)
{
    board_init();
    board_init_led_pins();
    board_init_usb_pins();

    intc_set_irq_priority(CONFIG_HPM_USBD_IRQn, 2);
    uartx_preinit();

    USER_Expansion_Init();

    chry_dap_init();
    while (1) {
        chry_dap_handle();
        chry_dap_usb2uart_handle();
        USER_Expansion_Loop();
    }
}
