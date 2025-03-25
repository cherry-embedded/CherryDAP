#include "hpm_romapi.h"
#include <elog.h>
#include <fal.h>
#include <hpm_l1c_drv.h>

typedef struct {
    XPI_Type *xpi_base;
    uint32_t base_addr;
    uint32_t sector_size;
    uint32_t opt_header;
    uint32_t opt0;
    uint32_t opt1;
    xpi_nor_config_t nor_config;
} nor_flash_config_t;

static nor_flash_config_t *nor_cfg;
ATTR_RAMFUNC
static int init(void) {

    nor_cfg = (nor_flash_config_t *) malloc(sizeof(nor_flash_config_t));
    nor_cfg->xpi_base = BOARD_APP_XPI_NOR_XPI_BASE;
    nor_cfg->base_addr = BOARD_FLASH_BASE_ADDRESS;
    nor_cfg->opt_header = BOARD_APP_XPI_NOR_CFG_OPT_HDR;
    nor_cfg->opt0 = BOARD_APP_XPI_NOR_CFG_OPT_OPT0;
    nor_cfg->opt1 = BOARD_APP_XPI_NOR_CFG_OPT_OPT1;

    xpi_nor_config_option_t option;

    option.header.U = nor_cfg->opt_header;
    option.option0.U = nor_cfg->opt0;
    option.option1.U = nor_cfg->opt1;
    hpm_stat_t status = rom_xpi_nor_auto_config(nor_cfg->xpi_base, &nor_cfg->nor_config, &option);
    if (status != status_success) {
        return status;
    }

    rom_xpi_nor_get_property(nor_cfg->xpi_base, &nor_cfg->nor_config, xpi_nor_property_sector_size, &nor_cfg->sector_size);
    __asm volatile("fence.i");
    return status_success;
}

ATTR_RAMFUNC
static int read(long offset, uint8_t *buf, size_t size) {
    uint32_t addr = hpm_internal_flash.addr + offset;

    disable_global_irq(CSR_MSTATUS_MIE_MASK);

    uint32_t aligned_start = HPM_L1C_CACHELINE_ALIGN_DOWN(addr);
    uint32_t aligned_end = HPM_L1C_CACHELINE_ALIGN_UP(addr + size);
    uint32_t aligned_size = aligned_end - aligned_start;

    (void) nor_cfg;
    l1c_dc_invalidate(aligned_start, aligned_size);

    memcpy(buf, (void *) addr, size);

    enable_global_irq(CSR_MSTATUS_MIE_MASK);

    return size;
}

ATTR_RAMFUNC
static int write(long offset, const uint8_t *buf, size_t size) {
    uint32_t addr = offset;

    disable_global_irq(CSR_MSTATUS_MIE_MASK);

    hpm_stat_t status = rom_xpi_nor_program(nor_cfg->xpi_base, xpi_xfer_channel_auto,
                                            &nor_cfg->nor_config, (const uint32_t *) buf, addr, size);
    if (status != status_success) {
        log_w("program failed");
    }

    enable_global_irq(CSR_MSTATUS_MIE_MASK);

    return size;
}

ATTR_RAMFUNC
static int erase(long offset, size_t size) {
    uint32_t addr = offset;

    disable_global_irq(CSR_MSTATUS_MIE_MASK);

    for (uint32_t start_addr = addr; start_addr < addr + size; start_addr += nor_cfg->sector_size) {
        hpm_stat_t status = rom_xpi_nor_erase_sector(nor_cfg->xpi_base, xpi_xfer_channel_auto, &nor_cfg->nor_config, start_addr);
        if (status != status_success) {
            log_w("erase sector failed");
        }
    }

    enable_global_irq(CSR_MSTATUS_MIE_MASK);

    return size;
}

const struct fal_flash_dev hpm_internal_flash = {
        .name = "hpm_internal",
        .addr = 0x80000000,
        .len = 1024 * 1024,
        .blk_size = 4 * 1024,
        .ops = {init, read, write, erase},
        .write_gran = 8,
};
