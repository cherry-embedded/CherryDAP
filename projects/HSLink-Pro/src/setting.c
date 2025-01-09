#include "setting.h"

#include <hpm_romapi.h>
#include "board.h"
#include "eeprom_emulation.h"
#include "hpm_nor_flash.h"

HSLink_Setting_t HSLink_Setting = {
    .boost = false,
    .swd_port_mode = PORT_MODE_SPI,
    .jtag_port_mode = PORT_MODE_SPI,
    .power = {
        .enable = false,
        .voltage = 3.3,
        .power_on = false,
    },
    .reset = RESET_NRST,
    .led = false,
    .led_brightness = 0,
};

#define SETTING_E2P_RX_SIZE        (512)
#define SETTING_E2P_BLOCK_SIZE_MAX (32)
#define SETTING_E2P_ERASE_SIZE     (4096)
#define SETTING_E2P_SECTOR_CNT     (32)
#define SETTING_E2P_MANEGE_SIZE    (SETTING_E2P_ERASE_SIZE * SETTING_E2P_SECTOR_CNT)
#define SETTING_E2P_MANAGE_OFFSET  (BOARD_FLASH_SIZE - SETTING_E2P_MANEGE_SIZE * 2)

static const uint32_t setting_eeprom_id = 114514;
static e2p_t e2p;

static uint32_t setting_e2p_read(uint8_t *buf, uint32_t addr, uint32_t size)
{
    return nor_flash_read(&e2p.nor_config, buf, addr, size);
}

static uint32_t setting_e2p_write(uint8_t *buf, uint32_t addr, uint32_t size)
{
    return nor_flash_write(&e2p.nor_config, buf, addr, size);
}

static void setting_e2p_erase(uint32_t start_addr, uint32_t size)
{
    nor_flash_erase(&e2p.nor_config, start_addr, size);
}

void Setting_Init(void)
{
    e2p.nor_config.xpi_base = BOARD_APP_XPI_NOR_XPI_BASE;
    e2p.nor_config.base_addr = BOARD_FLASH_BASE_ADDRESS;
    e2p.config.start_addr = e2p.nor_config.base_addr + SETTING_E2P_MANAGE_OFFSET;
    e2p.config.erase_size = SETTING_E2P_ERASE_SIZE;
    e2p.config.sector_cnt = SETTING_E2P_SECTOR_CNT;
    e2p.config.version = 0x4553; /* 'E' 'S' */
    e2p.nor_config.opt_header = BOARD_APP_XPI_NOR_CFG_OPT_HDR;
    e2p.nor_config.opt0 = BOARD_APP_XPI_NOR_CFG_OPT_OPT0;
    e2p.nor_config.opt1 = BOARD_APP_XPI_NOR_CFG_OPT_OPT1;
    e2p.config.flash_read = setting_e2p_read;
    e2p.config.flash_write = setting_e2p_write;
    e2p.config.flash_erase = setting_e2p_erase;

    nor_flash_init(&e2p.nor_config);
    e2p_config(&e2p);
    e2p_read(setting_eeprom_id, sizeof(HSLink_Setting_t), (uint8_t *)&HSLink_Setting);
}

void Setting_Save(void)
{
    e2p_write(setting_eeprom_id, sizeof(HSLink_Setting_t), (uint8_t *)&HSLink_Setting);
}
