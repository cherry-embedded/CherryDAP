#include "bootuf2.h"
#include "usb_config.h"
#include <board.h>
#include <hpm_l1c_drv.h>
#include <usb_log.h>

extern void msc_bootuf2_init(uint8_t busid, uint32_t reg_base);

static void jump_app(void)
{
    fencei();
    l1c_dc_disable();
    __asm("la a0, %0" ::"i"(CONFIG_BOOTUF2_APP_START + 4));
    __asm("jr a0");
}

int main(void)
{
    board_init();
    board_init_usb_pins();

    intc_set_irq_priority(CONFIG_HPM_USBD_IRQn, 2);
    printf("cherry usb msc device sample.\n");

    msc_bootuf2_init(0, CONFIG_HPM_USBD_BASE);

    while (1) {
        if (bootuf2_is_write_done()) {
            USB_LOG_INFO("write done\n");
            USB_LOG_INFO("Jump to application @0x%x(0x%x)\r\n", (CONFIG_BOOTUF2_APP_START + 4), *(volatile uint32_t *)(CONFIG_BOOTUF2_APP_START + 4));

            jump_app();
            while (1);
        }
    }
    return 0;
}
