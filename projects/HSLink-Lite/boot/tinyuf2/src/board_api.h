/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Ha Thach for Adafruit Industries
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * Copyright (c) 2022 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef BOARD_API_H
#define BOARD_API_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "board.h"

#define UF2_VOLUME_LABEL "HPMBOOT"
#define UF2_INDEX_URL "https://www.hpmicro.com"
#define UF2_PRODUCT_NAME "HSLink-Lite"
#define UF2_BOARD_ID "HSLink-Lite"

#define BOARD_UF2_FAMILY_ID 0x0A4D5048 /* HPM\n */
/* It has to match the application start address in link script */
#define BOARD_FLASH_APP_START (0x80020000UL)
/* Actual application starts at BOARD_FLASH_APP_START + 4, the first word is used for UF2 signature */
#define BOARD_UF2_APP_START (BOARD_FLASH_APP_START + 4)

#define UF2_AVAILABLE_BOARD_FLASH_SIZE (BOARD_FLASH_SIZE - (BOARD_FLASH_APP_START - BOARD_FLASH_BASE_ADDRESS))

#define USB_VID 0x34b7
#define USB_PID 0x1001
#define USB_MANUFACTURER  "HPMicro"
#define USB_PRODUCT       "HSLink-Lite"

#define UF2_BOOTLOADER_SAMPLE_COUNT 10
#define UF2_BOOTLOADER_PIN_GPIO_CTRL HPM_GPIO0
#define UF2_BOOTLOADER_PIN_GPIO_INDEX GPIO_DI_GPIOA
#define UF2_BOOTLOADER_PIN_GPIO_PIN 10
#define UF2_BOOTLOADER_PIN_ACTIVE 0

#ifndef TUF2_LOG
#define TUF2_LOG 0
#endif

/*--------------------------------------------------------------------*/
/* Basic API */
/*--------------------------------------------------------------------*/
#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/* Baudrate for UART if used */
#define BOARD_UART_BAUDRATE   BOARD_CONSOLE_BAUDRATE

/* Init basic peripherals such as clock, led indicator control (gpio, pwm etc ..) */
void uf2_board_init(void);

/* Reset board, not return */
void uf2_board_reset(void);

/* Write PWM duty value to LED */
void uf2_board_pwm_led_write(uint8_t value);

/* Write color to rgb strip */
void uf2_board_pwm_rgb_write(uint8_t *rgb);

/* Init uart hardware */
void uf2_board_uart_init(uint32_t baud_rate);

/* Send characters to UART for debugging */
int uf2_board_uart_write(void const *buf, int len);

/* start timer with ms interval */
void uf2_board_timer_start(uint32_t ms);

/* stop timer */
void uf2_board_timer_stop(void);

/* timer event handler, must be called by port/board */
void uf2_board_timer_handler(void);

/* Check if application is valid */
bool uf2_board_app_valid(void);

/* Jump to Application */
void uf2_board_app_jump(void);

/* Init USB hardware (not used for now) */
void uf2_board_usb_init(void);

/* Init DFU process, mostly for starting USB */
void uf2_board_dfu_init(void);

/* DFU is complete, should reset or jump to application mode and not return */
void uf2_board_dfu_complete(void);

/* Fill Serial Number and return its length (limit to 16 bytes) */
uint8_t uf2_board_usb_get_serial(uint8_t *serial_id);

/*--------------------------------------------------------------------*/
/* Flash API */
/*--------------------------------------------------------------------*/

/* Initialize flash for DFU */
void uf2_board_flash_init(void);

/* Get size of flash */
uint32_t uf2_board_flash_size(void);

/* Read from flash */
void uf2_board_flash_read (uint32_t addr, void *buffer, uint32_t len);

/* Write to flash */
void uf2_board_flash_write(uint32_t addr, void const *data, uint32_t len);

/* Flush/Sync flash contents */
void uf2_board_flash_flush(void);

/* Erase application */
void uf2_board_flash_erase_app(void);

/* perform self-update on bootloader */
void uf2_board_self_update(const uint8_t *bootloader_bin, uint32_t bootloader_len);

/* check if user wants to enter bootloader mode */
bool uf2_board_enter_bootloader(void);

/* timer handler */
void uf2_board_timer_handler(void);
#if defined(__cplusplus)
}
#endif /* __cplusplus */
/*--------------------------------------------------------------------*/
/* LOG */
/*--------------------------------------------------------------------*/
#if TUF2_LOG

#include <stdio.h>

#ifndef tuf2_printf
#define tuf2_printf printf
#endif

/* Log with debug level 1 */
#define TUF2_LOG1               tuf2_printf
#define TUF2_LOG1_MEM           /* tu_print_mem */
#define TUF2_LOG1_VAR(_x)       /* tu_print_var((uint8_t const*)(_x), sizeof(*(_x))) */
#define TUF2_LOG1_INT(_x)       tuf2_printf(#_x " = %ld\n", (uint32_t) (_x))
#define TUF2_LOG1_HEX(_x)       tuf2_printf(#_x " = %lX\n", (uint32_t) (_x))
#define TUF2_LOG1_LOCATION()    tuf2_printf("%s: %d:\r\n", __PRETTY_FUNCTION__, __LINE__)
#define TUF2_LOG1_FAILED()      tuf2_printf("%s: %d: Failed\r\n", __PRETTY_FUNCTION__, __LINE__)

/* Log with debug level 2 */
#if CFG_TUSB_DEBUG > 1
#define TUF2_LOG2             TUF2_LOG1
#define TUF2_LOG2_MEM         TUF2_LOG1_MEM
#define TUF2_LOG2_VAR         TUF2_LOG1_VAR
#define TUF2_LOG2_INT         TUF2_LOG1_INT
#define TUF2_LOG2_HEX         TUF2_LOG1_HEX
#define TUF2_LOG2_LOCATION()  TUF2_LOG1_LOCATION()
#endif

#endif /* TUF2_LOG */

#ifndef TUF2_LOG1
#define TUF2_LOG1(...)
#define TUF2_LOG1_MEM(...)
#define TUF2_LOG1_VAR(...)
#define TUF2_LOG1_INT(...)
#define TUF2_LOG1_HEX(...)
#define TUF2_LOG1_LOCATION()
#define TUF2_LOG1_FAILED()
#endif

#ifndef TUF2_LOG2
#define TUF2_LOG2(...)
#define TUF2_LOG2_MEM(...)
#define TUF2_LOG2_VAR(...)
#define TUF2_LOG2_INT(...)
#define TUF2_LOG2_HEX(...)
#define TUF2_LOG2_LOCATION()
#endif

/*--------------------------------------------------------------------*/
/* not part of board API, move to its own file later */
/*--------------------------------------------------------------------*/

enum {
    STATE_BOOTLOADER_STARTED = 0,/*/< STATE_BOOTLOADER_STARTED */
    STATE_USB_PLUGGED,           /*/< STATE_USB_PLUGGED */
    STATE_USB_UNPLUGGED,         /*/< STATE_USB_UNPLUGGED */
    STATE_WRITING_STARTED,       /*/< STATE_WRITING_STARTED */
    STATE_WRITING_FINISHED,      /*/< STATE_WRITING_FINISHED */
};

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */
void indicator_set(uint32_t state);

static inline void rgb_brightness(uint8_t out[3], uint8_t const in[3], uint8_t brightness)
{
    for (uint32_t i = 0; i < 3; i++) {
        out[i] = (in[i]*brightness) >> 8;
    }
}
#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* BOARD_API_H */
