/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
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
 */
/*
 * Copyright (c) 2022 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "board_api.h"
#include "uf2.h"
#include "tusb.h"

/*--------------------------------------------------------------------*/
/* MACRO CONSTANT TYPEDEF PROTYPES */
/*--------------------------------------------------------------------*/
uint8_t RGB_USB_UNMOUNTED[] = { 0xff, 0x00, 0x00 }; /* Red */
uint8_t RGB_USB_MOUNTED[] = { 0x00, 0xff, 0x00 };   /* Green */
uint8_t RGB_WRITING[] = { 0xcc, 0x66, 0x00 };
uint8_t RGB_ENTER_BOOTLOADER[] = { 0x80, 0x00, 0xff }; /* Purple */
uint8_t RGB_UNKNOWN[] = { 0x00, 0x00, 0x88 };          /* for debug */
uint8_t RGB_OFF[] = { 0x00, 0x00, 0x00 };

static volatile uint32_t _timer_count;

static bool check_dfu_mode(void);

int main(void)
{
    uf2_board_init();

    if (BOARD_TUD_RHPORT == 0) {
        board_init_usb(HPM_USB0);
#ifdef HPM_USB1
    } else if (BOARD_TUD_RHPORT == 1) {
        board_init_usb(HPM_USB1);
#endif
    } else {
        printf("Don't support HPM_USB%d!\n", BOARD_TUD_RHPORT);
        while (1) {
            ;
        }
    }

    TU_LOG1("TinyUF2\r\n");

    /* if not DFU mode, jump to App */
    if (!check_dfu_mode()) {
        TU_LOG1("Jump to application @0x%x(0x%x)\r\n", BOARD_UF2_APP_START, *(volatile uint32_t *)BOARD_UF2_APP_START);
        uf2_board_app_jump();
        while (1) {
        }
    }

    TU_LOG1("Start DFU mode\r\n");
    uf2_board_dfu_init();
    uf2_board_flash_init();
    uf2_init();

    /* init device stack on configured roothub port */
    tusb_rhport_init_t dev_init = {
        .role = TUSB_ROLE_DEVICE,
        .speed = TUSB_SPEED_AUTO,
    };
    tusb_init(BOARD_TUD_RHPORT, &dev_init);

    indicator_set(STATE_USB_UNPLUGGED);

#if (CFG_TUSB_OS == OPT_OS_NONE)
    while (1) {
        tud_task();
    }
#endif
}

/* return true if start DFU mode, else App mode */
static bool check_dfu_mode(void)
{
    if (!uf2_board_enter_bootloader()) {
        return true;
    }

    if (!uf2_board_app_valid()) {
        return true;
    }
    return false;
}

/*--------------------------------------------------------------------*/
/* Device callbacks */
/*--------------------------------------------------------------------*/

/* Invoked when device is plugged and configured */
void tud_mount_cb(void)
{
    indicator_set(STATE_USB_PLUGGED);
}

/* Invoked when device is unplugged */
void tud_umount_cb(void)
{
    indicator_set(STATE_USB_UNPLUGGED);
}

/*--------------------------------------------------------------------*/
/* USB HID */
/*--------------------------------------------------------------------*/

/* Invoked when received GET_REPORT control request */
/* Application must fill buffer report's content and return its length. */
/* Return zero will cause the stack to STALL request */
uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
    /* TODO not Implemented */
    (void)itf;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)reqlen;

    return 0;
}

/* Invoked when received SET_REPORT control request or */
/* received data on OUT endpoint (Report ID = 0, Type = 0) */
void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
    /* This example doesn't use multiple report and report ID */
    (void)itf;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)bufsize;
}

/*--------------------------------------------------------------------*/
/* Indicator */
/*--------------------------------------------------------------------*/

static uint32_t _indicator_state = STATE_BOOTLOADER_STARTED;
static uint8_t _indicator_rgb[3];

void indicator_set(uint32_t state)
{
    _indicator_state = state;
    switch (state) {
    case STATE_USB_UNPLUGGED:
        uf2_board_timer_start(1);
        memcpy(_indicator_rgb, RGB_USB_UNMOUNTED, 3);
        uf2_board_pwm_rgb_write(_indicator_rgb);
        break;

    case STATE_USB_PLUGGED:
        uf2_board_timer_start(5);
        memcpy(_indicator_rgb, RGB_USB_MOUNTED, 3);
        uf2_board_pwm_rgb_write(_indicator_rgb);
        break;

    case STATE_WRITING_STARTED:
        uf2_board_timer_start(25);
        memcpy(_indicator_rgb, RGB_WRITING, 3);
        break;

    case STATE_WRITING_FINISHED:
        uf2_board_timer_stop();
        uf2_board_pwm_rgb_write(RGB_WRITING);
        break;
    default:
        break; /* nothing to do */
    }
}

void uf2_board_timer_handler(void)
{
    bool is_on;

    _timer_count++;

    switch (_indicator_state) {
    case STATE_USB_UNPLUGGED:
    case STATE_USB_PLUGGED:
        uf2_board_pwm_rgb_write(_indicator_rgb);
        break;

    case STATE_WRITING_STARTED:
        /* Fast toggle with both LED and RGB */
        is_on = _timer_count & 0x01;
        /* blink RGB if available */
        uf2_board_pwm_rgb_write(is_on ? _indicator_rgb : RGB_OFF);
        break;

    default:
        break; /* nothing to do */
    }
}
