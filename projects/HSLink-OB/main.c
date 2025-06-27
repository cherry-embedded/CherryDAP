#include "board.h"
#include "dap_main.h"
#include "setting.h"
#include "hpm_dma_mgr.h"
#include "usb2uart.h"

HSLink_Setting_t HSLink_Setting = {
    .boost = false,
    .swd_port_mode = PORT_MODE_SPI,
    .jtag_port_mode = PORT_MODE_SPI,
    .power = {
        .vref = 3.3,
        .power_on = false,
        .port_on = false,
    },
    .reset = 0,
    .led = false,
    .led_brightness = 0,
};

HSLink_Lazy_t HSLink_Global = {
    .reset_level = false,
};

static inline void SWDIO_DIR_Init(void)
{
    HPM_IOC->PAD[SWDIO_DIR].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);

    gpiom_set_pin_controller(HPM_GPIOM, GPIO_GET_PORT_INDEX(SWDIO_DIR), GPIO_GET_PIN_INDEX(SWDIO_DIR), PIN_GPIOM);
    gpio_set_pin_output(PIN_GPIO, GPIO_GET_PORT_INDEX(SWDIO_DIR), GPIO_GET_PIN_INDEX(SWDIO_DIR));
    gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(SWDIO_DIR), GPIO_GET_PIN_INDEX(SWDIO_DIR), 1);
}

extern void uartx_preinit(void);

int main(void)
{
    board_init();
    board_init_usb(HPM_USB0);
    dma_mgr_init();

    SWDIO_DIR_Init();

    intc_set_irq_priority(CONFIG_HPM_USBD_IRQn, 2);
    uartx_preinit();

    chry_dap_init(0, HPM_USB0_BASE);
    while (1) {
        chry_dap_handle();
        chry_dap_usb2uart_handle();
        usb2uart_handler();
    }
}
