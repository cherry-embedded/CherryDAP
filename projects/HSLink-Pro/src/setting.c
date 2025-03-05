#include "setting.h"

#include <hpm_romapi.h>
#include "board.h"
#include "eeprom_emulation.h"
#include "hpm_nor_flash.h"
#include "BL_Setting_Common.h"

HSLink_Setting_t HSLink_Setting = {
    .boost = false,
    .swd_port_mode = PORT_MODE_SPI,
    .jtag_port_mode = PORT_MODE_SPI,
    .power = {
        .voltage = 3.3,
        .power_on = false,
        .port_on = false,
    },
    .reset = RESET_NRST,
    .led = false,
    .led_brightness = 0,
};

Setting_Version_t HSLink_Hardware_Version;

ATTR_PLACE_AT(".bl_setting")
BL_Setting_t bl_setting;

#define APP_OFFSET (0x20000)
#define SETTING_E2P_RX_SIZE        (512)
#define SETTING_E2P_BLOCK_SIZE_MAX (32)
#define SETTING_E2P_ERASE_SIZE     (4096)
#define SETTING_E2P_SECTOR_CNT     (32)
#define SETTING_E2P_MANEGE_SIZE    (SETTING_E2P_ERASE_SIZE * SETTING_E2P_SECTOR_CNT)
#define SETTING_E2P_MANAGE_OFFSET  (BOARD_FLASH_SIZE - APP_OFFSET - SETTING_E2P_MANEGE_SIZE * 2)

static const uint32_t HARDWARE_VER_ADDR = 69;

static const char *e2p_name = "HSP";
static uint32_t setting_eeprom_id;
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

static void print_param(void)
{
    printf("magic: %08X\n", HSLink_Setting.magic);
    printf("boost: %d\n", HSLink_Setting.boost);
    printf("nickname: %s\n", HSLink_Setting.nickname);
}

static Setting_Version_t get_hardware_version(void)
{
    uint32_t version = ROM_API_TABLE_ROOT->otp_driver_if->read_from_shadow(HARDWARE_VER_ADDR);
    Setting_Version_t ver;
    ver.major = (version >> 24) & 0xFF;
    ver.minor = (version >> 16) & 0xFF;
    ver.patch = (version >> 8) & 0xFF;
    ver.reserved = version & 0xFF;

    return ver;
}

void Setting_Init(void)
{
    HSLink_Hardware_Version = get_hardware_version();

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

    disable_global_irq(CSR_MSTATUS_MIE_MASK);
    nor_flash_init(&e2p.nor_config);
    e2p_config(&e2p);
    setting_eeprom_id = e2p_generate_id(e2p_name);
    HSLink_Setting_t temp;
    e2p_read(setting_eeprom_id, sizeof(HSLink_Setting_t), (uint8_t *) &temp);
    if (temp.magic != SETTING_MAGIC)
    {
        // 第一次烧录，使用默认设置
        printf("First boot, use default setting\n");
        Setting_Save();
        return;
    }
    enable_global_irq(CSR_MSTATUS_MIE_MASK);
    memcpy(&HSLink_Setting, &temp, sizeof(HSLink_Setting_t));
    print_param();
}

void Setting_Save(void)
{
    disable_global_irq(CSR_MSTATUS_MIE_MASK);
    HSLink_Setting.magic = SETTING_MAGIC;
    e2p_write(setting_eeprom_id, sizeof(HSLink_Setting_t), (uint8_t *) &HSLink_Setting);
    print_param();
    enable_global_irq(CSR_MSTATUS_MIE_MASK);
}
