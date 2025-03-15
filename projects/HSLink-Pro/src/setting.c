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
#define SETTING_E2P_MANEGE_SIZE    (SETTING_E2P_ERASE_SIZE * SETTING_E2P_SECTOR_CNT)    // 128K
#define SETTING_E2P_MANAGE_OFFSET  (BOARD_FLASH_SIZE - APP_OFFSET - SETTING_E2P_MANEGE_SIZE * 2)    // 1M - 0x20000 - 256K = 640K

static const uint32_t HARDWARE_VER_ADDR = 70;

static const char *e2p_name = "HSP";
static const char *e2p_hw_ver_name = "hw_ver";
static e2p_t e2p;

static uint32_t setting_e2p_read(uint8_t *buf, uint32_t addr, uint32_t size) {
    return nor_flash_read(&e2p.nor_config, buf, addr, size);
}

static uint32_t setting_e2p_write(uint8_t *buf, uint32_t addr, uint32_t size) {
    return nor_flash_write(&e2p.nor_config, buf, addr, size);
}

static void setting_e2p_erase(uint32_t start_addr, uint32_t size) {
    nor_flash_erase(&e2p.nor_config, start_addr, size);
}

static void print_param(void) {
    printf("magic: %08X\n", HSLink_Setting.magic);
    printf("boost: %d\n", HSLink_Setting.boost);
    printf("nickname: %s\n", HSLink_Setting.nickname);
}

static void e2p_init() {
    disable_global_irq(CSR_MSTATUS_MIE_MASK);

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

    enable_global_irq(CSR_MSTATUS_MIE_MASK);
}

static void load_hardware_version(void) {
    // first version and diy devices did not write version into otp
    // we should read version from flash firstly
    // if not exist, we should read from otp
    // if not exist either, we marked it as 0.0.0
    // we would also notice user in upper to write version into flash
    uint32_t id = e2p_generate_id(e2p_hw_ver_name);
    if (E2P_ERROR_BAD_ID != e2p_read(id, sizeof(Setting_Version_t), (uint8_t *) &HSLink_Hardware_Version)) {
//        printf("Load hardware version from flash\r\n");
        return;
    }
//    printf("Load hardware version from otp\r\n");
    uint32_t version = ROM_API_TABLE_ROOT->otp_driver_if->read_from_shadow(HARDWARE_VER_ADDR);
    HSLink_Hardware_Version.major = (version >> 24) & 0xFF;
    HSLink_Hardware_Version.minor = (version >> 16) & 0xFF;
    HSLink_Hardware_Version.patch = (version >> 8) & 0xFF;
    HSLink_Hardware_Version.reserved = version & 0xFF;
}

static void load_settings(void) {
    uint32_t setting_eeprom_id = e2p_generate_id(e2p_name);
    HSLink_Setting_t temp = {0};
    e2p_read(setting_eeprom_id, sizeof(HSLink_Setting_t), (uint8_t *) &temp);
    if (temp.magic != SETTING_MAGIC) {
        // 第一次烧录，使用默认设置
        printf("First boot, use default setting\n");
        Setting_Save();
        return;
    } else {
        memcpy(&HSLink_Setting, &temp, sizeof(HSLink_Setting_t));
    }
}

void Setting_Init(void) {
    e2p_init();

    load_hardware_version();

    load_settings();

    printf("Hardware version: %d.%d.%d\n",
           HSLink_Hardware_Version.major,
           HSLink_Hardware_Version.minor,
           HSLink_Hardware_Version.patch);

    print_param();
}

void Setting_Save(void) {
    disable_global_irq(CSR_MSTATUS_MIE_MASK);
    uint32_t setting_eeprom_id = e2p_generate_id(e2p_name);
    HSLink_Setting.magic = SETTING_MAGIC;
    e2p_write(setting_eeprom_id, sizeof(HSLink_Setting_t), (uint8_t *) &HSLink_Setting);
    enable_global_irq(CSR_MSTATUS_MIE_MASK);
    printf("settings update to ");
    print_param();
}

void Setting_SaveHardwareVersion(Setting_Version_t hw_ver) {
    disable_global_irq(CSR_MSTATUS_MIE_MASK);
    uint32_t id = e2p_generate_id(e2p_hw_ver_name);
    e2p_write(id, sizeof(Setting_Version_t), (uint8_t *) &hw_ver);
    enable_global_irq(CSR_MSTATUS_MIE_MASK);
    printf("hardware version update to %d.%d.%d\n", hw_ver.major, hw_ver.minor, hw_ver.patch);
}

uint8_t Setting_IsHardwareVersion(uint8_t major, uint8_t minor, uint8_t patch) {
    if (major == UINT8_MAX) {
        return 1;
    } else if (minor == UINT8_MAX) {
        return HSLink_Hardware_Version.major == major;
    } else if (patch == UINT8_MAX) {
        return HSLink_Hardware_Version.major == major && HSLink_Hardware_Version.minor == minor;
    } else {
        return HSLink_Hardware_Version.major == major && HSLink_Hardware_Version.minor == minor &&
               HSLink_Hardware_Version.patch == patch;
    }
}

uint8_t Setting_GetSRSTLevel(void) {
    uint8_t level = 1;
    if (Setting_IsHardwareVersion(1, 2, UINT8_MAX)) {
        level = 0;
    }
    return level;
}
