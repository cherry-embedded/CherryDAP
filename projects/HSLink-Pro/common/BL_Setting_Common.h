#ifndef BL_SETTING_COMMON_H
#define BL_SETTING_COMMON_H

#include <stdint.h>

typedef enum {
    BL_SETTING_MAGIC = 0xB7B7B7B7,

    BL_SETTING_MAGIC_INVALID = 0xFFFFFFFF,
} BL_SETTING_MAGIC_t;

typedef struct {
    uint32_t magic; // 这个段未初始化，使用magic来判断是否已经初始化
    union {
        uint8_t data0;
        struct {
            uint8_t is_update : 1; // 判断是否需要进入bootloader
        };
    };

} BL_Setting_t;

#endif