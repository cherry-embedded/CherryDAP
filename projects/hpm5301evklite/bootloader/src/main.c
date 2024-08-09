#include "bootuf2.h"
#include "usb_config.h"
#include <board.h>
#include <hpm_gpio_drv.h>
#include <hpm_gpiom_drv.h>
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

static void bootloader_button_init(void)
{
    // BOOT1(PA03)
    HPM_IOC->PAD[IOC_PAD_PA03].FUNC_CTL = IOC_PA03_FUNC_CTL_GPIO_A_03;

    gpiom_set_pin_controller(HPM_GPIOM, GPIOM_ASSIGN_GPIOA, 3, gpiom_soc_gpio0);
    gpio_set_pin_input(HPM_GPIO0, GPIO_OE_GPIOA, 3);
    gpio_disable_pin_interrupt(HPM_GPIO0, GPIO_IE_GPIOA, 3);
}

static bool bootloader_button_pressed(void)
{
    if (gpio_read_pin(HPM_GPIO0, GPIO_DI_GPIOA, 3) == 1)
    {
        board_delay_ms(5);
        if (gpio_read_pin(HPM_GPIO0, GPIO_DI_GPIOA, 3) == 1)
        {
            return true;
        }
    }
    return false;
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

static void show_logo(void)
{
    const char str[] = {
        "  _    _  _____ _      _       _      _____           \n"
        " | |  | |/ ____| |    (_)     | |    |  __ \\          \n"
        " | |__| | (___ | |     _ _ __ | | __ | |__) | __ ___  \n"
        " |  __  |\\___ \\| |    | | '_ \\| |/ / |  ___/ '__/ _ \\ \n"
        " | |  | |____) | |____| | | | |   <  | |   | | | (_) |\n"
        " |_|  |_|_____/|______|_|_| |_|_|\\_\\ |_|   |_|  \\___/ \n"
        "                                                      \n"
        "                                                      \n"
        ""};
    printf("%s", str);
}

int main(void)
{
    board_init();
    show_logo();
    board_init_usb_pins();
    bootloader_button_init();

    if (!bootloader_button_pressed() && app_valid())
    {
        USB_LOG_INFO("Jump to application @0x%x(0x%x)\r\n", CONFIG_BOOTUF2_APP_START, *(volatile uint32_t *)CONFIG_BOOTUF2_APP_START);
        jump_app();
        while (1)
            ;
    }

    intc_set_irq_priority(CONFIG_HPM_USBD_IRQn, 2);
    printf("cherry usb msc device sample.\n");

    msc_bootuf2_init(0, CONFIG_HPM_USBD_BASE);

    while (1)
    {
        if (bootuf2_is_write_done())
        {
            USB_LOG_INFO("Update success! Jump to application.\n");

            jump_app();
            while (1)
                ;
        }
    }
    return 0;
}
