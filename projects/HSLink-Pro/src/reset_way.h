//
// Created by yekai on 2025/8/5.
//

#ifndef RESET_WAY_H
#define RESET_WAY_H

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

uint8_t software_reset(void);
void por_reset(void);
void srst_reset(void);

#ifdef __cplusplus
    }
#endif

#endif //RESET_WAY_H
