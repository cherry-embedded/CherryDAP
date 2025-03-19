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

ATTR_PLACE_AT(".bl_setting")
BL_Setting_t bl_setting;

static const char *e2p_name = "HSP";

static void print_param(void) {
    printf("magic: %08X\n", HSLink_Setting.magic);
    printf("boost: %d\n", HSLink_Setting.boost);
    printf("nickname: %s\n", HSLink_Setting.nickname);
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
    load_settings();

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

void Setting_SaveHardwareVersion(version_t hw_ver) {
    disable_global_irq(CSR_MSTATUS_MIE_MASK);
    uint32_t id = e2p_generate_id(e2p_hw_ver_name);
    e2p_write(id, sizeof(version_t), (uint8_t *) &hw_ver);
    enable_global_irq(CSR_MSTATUS_MIE_MASK);
    printf("hardware version update to %d.%d.%d\n", hw_ver.major, hw_ver.minor, hw_ver.patch);
}

uint8_t Setting_GetSRSTLevel(void) {
    uint8_t level = 1;
    if (CheckHardwareVersion(1, 2, UINT8_MAX)) {
        level = 0;
    }
    return level;
}
