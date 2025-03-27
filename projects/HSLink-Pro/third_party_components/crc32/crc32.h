//
// Created by yekai on 2025/3/23.
//

#ifndef HSLINK_PRO_CRC32_H
#define HSLINK_PRO_CRC32_H

#include "cstdint"

uint32_t CRC_CalcArray_Software(uint8_t *data, uint32_t len, uint32_t crc32=0xFFFFFFFF);

#endif //HSLINK_PRO_CRC32_H
