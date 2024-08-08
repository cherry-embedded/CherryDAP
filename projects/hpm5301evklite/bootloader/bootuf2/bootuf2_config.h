/*
* Copyright (c) 2024, sakumisu
*
* SPDX-License-Identifier: Apache-2.0
*/
#ifndef BOOTUF2_CONFIG_H
#define BOOTUF2_CONFIG_H

#define CONFIG_PRODUCT            "CherryDAP"
#define CONFIG_BOARD              "HSLink Pro"
#define CONFIG_BOOTUF2_INDEX_URL  "https://github.com/cherry-embedded/CherryDAP"
#define CONFIG_BOOTUF2_JOIN_URL   "http://qm.qq.com/cgi-bin/qm/qr?_wv=1027&k=NXGLdOOrFypXfBoAmh_eLwmcAZ2doqxJ&authKey=CDerxhytwBfYn1jHcfmBwOOl2B73vRxGmPJ0utVTFrnjfwfTikpzJbYUUPhyicmK&noverify=0&group_code=975779851"

#define CONFIG_BOOTUF2_CACHE_SIZE         4096
#define CONFIG_BOOTUF2_SECTOR_SIZE        512
#define CONFIG_BOOTUF2_SECTOR_PER_CLUSTER 2
#define CONFIG_BOOTUF2_SECTOR_RESERVED    1
#define CONFIG_BOOTUF2_NUM_OF_FAT         2
#define CONFIG_BOOTUF2_ROOT_ENTRIES       64

#define CONFIG_BOOTUF2_FAMILYID      0x0A4D5048
#define CONFIG_BOOTUF2_FLASHMAX      (1*1024*1024 - 0x20000)
#define CONFIG_BOOTUF2_PAGE_COUNTMAX 1024

#define CONFIG_BOOTUF2_APP_START (0x80020000UL)

#endif