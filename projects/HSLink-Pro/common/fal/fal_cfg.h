/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-05-17     armink       the first version
 */

#ifndef _FAL_CFG_H_
#define _FAL_CFG_H_

#include <board.h>
#include <fal.h>


/* ===================== Flash device Configuration ========================= */
extern const struct fal_flash_dev hpm_internal_flash;

/* flash device table */
#define FAL_FLASH_DEV_TABLE      \
    {                            \
            &hpm_internal_flash, \
    }
/* ====================== Partition Configuration ========================== */
/* Flash tabel
 * empty    0           - 0x400
 * bl       0x400       - 128 * 1024 约128K
 * app      128 * 1024  - 880 * 1024 752K
 * flashdb  880 * 1024  - 896 * 1024 16K
 * bl_b     896 * 1024  - 1024 * 1024 128K
 * */
#ifdef FAL_PART_HAS_TABLE_CFG
/* partition table */
// clang-format off
//  magicword               分区名           Flash 设备名  偏移地址   大小
#define FAL_PART_TABLE                                                              \
{                                                                               \
    {FAL_PART_MAGIC_WORD,   "bl",       "hpm_internal",     0x400,          127 * 1024,     0},       \
    {FAL_PART_MAGIC_WORD,   "app",      "hpm_internal",     128 * 1024,     752 * 1024,     0},       \
    {FAL_PART_MAGIC_WORD,   "flashdb",  "hpm_internal",     880 * 1024,     16 * 1024,      0},       \
    {FAL_PART_MAGIC_WORD,   "bl_b",     "hpm_internal",     896 * 1024,     128 * 1024,     0},       \
}
#else
#define FAL_PART_TABLE_FLASH_DEV_NAME "hpm_internal"
#define FAL_PART_TABLE_END_OFFSET (0x3200)
#endif /* FAL_PART_HAS_TABLE_CFG */
// clang-format on

extern struct fal_partition *bl_part;
extern struct fal_partition *app_part;
extern struct fal_partition *flashdb_part;
extern struct fal_partition *bl_b_part;
void fal_parts_init();

#endif /* _FAL_CFG_H_ */
