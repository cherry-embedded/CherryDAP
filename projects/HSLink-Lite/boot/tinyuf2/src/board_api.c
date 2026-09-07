/*
 * Copyright (c) 2022 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "board_api.h"
#include "hpm_romapi.h"
#include "hpm_ppor_drv.h"
#include "hpm_l1c_drv.h"
#include "hpm_gpio_drv.h"


void uf2_board_init(void)
{
    board_init();
    board_init_gpio_pins();
}

void uf2_board_app_jump(void)
{
    fencei();
    l1c_dc_disable();
    __asm("la a0, %0" ::"i"(BOARD_FLASH_APP_START + 4));
    __asm("jr a0");
}

bool uf2_board_app_valid(void)
{
    if (((volatile uint32_t const*) BOARD_FLASH_APP_START)[0] != BOARD_UF2_SIGNATURE) {
        return false;
    }

    return true;
}

void uf2_board_dfu_complete(void)
{
    ppor_sw_reset(HPM_PPOR, 10);
}

void uf2_board_dfu_init(void)
{
}

static xpi_nor_config_t s_xpi_nor_config;

void uf2_board_flash_init(void)
{
    xpi_nor_config_option_t option;
    option.header.U = BOARD_APP_XPI_NOR_CFG_OPT_HDR;
    option.option0.U = BOARD_APP_XPI_NOR_CFG_OPT_OPT0;
    option.option1.U = BOARD_APP_XPI_NOR_CFG_OPT_OPT1;
    rom_xpi_nor_auto_config(BOARD_APP_XPI_NOR_XPI_BASE, &s_xpi_nor_config, &option);
}

#define NO_CACHE        0xffffffff
#define SECTOR_SIZE     (4*1024)
static uint32_t _flash_page_addr = NO_CACHE;
static uint8_t  _flash_cache[SECTOR_SIZE] __attribute__((aligned(HPM_L1C_CACHELINE_SIZE)));

void uf2_board_flash_flush(void)
{

    hpm_stat_t status;

    if (_flash_page_addr == NO_CACHE) {
        return;
    }

    printf("Erase and Write at address = 0x%08lX\r\n", _flash_page_addr);

    /* Skip if data is the same */
    if (memcmp(_flash_cache, (void *) _flash_page_addr, SECTOR_SIZE) != 0) {
        uint32_t const sector_addr = (_flash_page_addr - BOARD_FLASH_BASE_ADDRESS);

        disable_global_irq(CSR_MSTATUS_MIE_MASK);
        status = rom_xpi_nor_erase(BOARD_APP_XPI_NOR_XPI_BASE, xpi_xfer_channel_auto, &s_xpi_nor_config,
                sector_addr, SECTOR_SIZE);
        enable_global_irq(CSR_MSTATUS_MIE_MASK);
        l1c_dc_invalidate(_flash_page_addr, SECTOR_SIZE);
        if (status != status_success) {
            printf("Erase failed: status = %ld!\r\n", status);
            return;
        }

        l1c_dc_writeback(_flash_page_addr, SECTOR_SIZE);
        disable_global_irq(CSR_MSTATUS_MIE_MASK);
        status = rom_xpi_nor_program(BOARD_APP_XPI_NOR_XPI_BASE, xpi_xfer_channel_auto, &s_xpi_nor_config,
                (uint32_t *)_flash_cache, sector_addr, SECTOR_SIZE);
        enable_global_irq(CSR_MSTATUS_MIE_MASK);
        if (status != status_success) {
            printf("Page program failed: status = %ld!\r\n", status);
            return;
        }


        l1c_dc_invalidate(_flash_page_addr, SECTOR_SIZE);
    }

    _flash_page_addr = NO_CACHE;
}


void uf2_board_flash_read(uint32_t addr, void *buffer, uint32_t len)
{
    rom_xpi_nor_read(BOARD_APP_XPI_NOR_XPI_BASE, xpi_xfer_channel_auto, &s_xpi_nor_config, buffer, addr, len);
}

uint32_t uf2_board_flash_size(void)
{
    return ((UF2_AVAILABLE_BOARD_FLASH_SIZE) > 8 << 20) ? 8 << 20 : UF2_AVAILABLE_BOARD_FLASH_SIZE;
}

void uf2_board_flash_write(uint32_t addr, void const *src, uint32_t len)
{
    uint32_t const page_addr = addr & ~(SECTOR_SIZE - 1);

    if (page_addr != _flash_page_addr) {
        /* Write out anything in cache before overwriting it. */
        uf2_board_flash_flush();

        _flash_page_addr = page_addr;

        /* Copy the current contents of the entire page into the cache. */
        memcpy(_flash_cache, (void *) page_addr, SECTOR_SIZE);
    }

    /* Overwrite part or all of the page cache with the src data. */
    memcpy(_flash_cache + (addr & (SECTOR_SIZE - 1)), src, len);
}

void uf2_board_pwm_rgb_write(uint8_t *rgb)
{
}

void uf2_board_pwm_led_write(uint8_t value)
{
}

void uf2_board_timer_start(uint32_t d)
{
}

void uf2_board_timer_stop(void)
{
}

uint8_t uf2_board_usb_get_serial(uint8_t *id)
{
    const uint8_t s[] = "HPMicro Semi SH";
    memcpy(id, s, sizeof(s));
    return 16;
}

bool uf2_board_enter_bootloader(void)
{
    uint32_t counter = 0;
    gpio_set_pin_input(UF2_BOOTLOADER_PIN_GPIO_CTRL, UF2_BOOTLOADER_PIN_GPIO_INDEX, UF2_BOOTLOADER_PIN_GPIO_PIN);
    for (uint8_t i = 0; i < UF2_BOOTLOADER_SAMPLE_COUNT; i++) {
        if (UF2_BOOTLOADER_PIN_ACTIVE == gpio_read_pin(UF2_BOOTLOADER_PIN_GPIO_CTRL, UF2_BOOTLOADER_PIN_GPIO_INDEX, UF2_BOOTLOADER_PIN_GPIO_PIN)) {
            counter++;
        }
        board_delay_ms(10);
    }
    if (counter > 0.6 * UF2_BOOTLOADER_SAMPLE_COUNT) {
        return true;
    }
    return false;
}

