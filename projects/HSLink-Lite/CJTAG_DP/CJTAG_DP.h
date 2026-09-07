/*
 * Copyright (c) 2025 runcheng,lu
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef CJTAG_DP_H_
#define CJTAG_DP_H_

#include "DAP_config.h"
#include "DAP.h"

#ifndef USE_SHORT_CONNECT_SEQ
#define USE_SHORT_CONNECT_SEQ 0
#endif

#ifndef USE_JLINK_STANDARD_SEQUENCE
#define USE_JLINK_STANDARD_SEQUENCE 1
#endif

#ifndef USE_OSCAN1_NOT_LOGIC_RESET
#define USE_OSCAN1_NOT_LOGIC_RESET 0
#endif

// CJTAG/TCK I/O pin -------------------------------------

/** CJTAG/TCK I/O pin: Get Input.
\return Current status of the CJTAG/TCK DAP hardware I/O pin.
*/
__STATIC_FORCEINLINE uint32_t PIN_CJTAG_TCK_IN(void)
{
    uint32_t sta =  gpio_read_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(PIN_TCK), GPIO_GET_PIN_INDEX(PIN_TCK));
    __asm volatile("fence io, io");
    return sta;
}

/** CJTAG/TCK I/O pin: Set Output to High
Set the CJTAG/TCK DAP hardware I/O pin to high level.;
*/
__STATIC_FORCEINLINE void PIN_CJTAG_TCK_SET(void)
{
    gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(PIN_TCK), GPIO_GET_PIN_INDEX(PIN_TCK), true);
    __asm volatile("fence io, io");
}

/** CJTAG/TCK I/O pin: Set Output to Low.
Set the CJTAG/TCK DAP hardware I/O pin to low level.
*/
__STATIC_FORCEINLINE void PIN_CJTAG_TCK_CLR(void)
{
    gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(PIN_TCK), GPIO_GET_PIN_INDEX(PIN_TCK), false);
    __asm volatile("fence io, io");
}

__STATIC_FORCEINLINE void PIN_CJTAG_TMSC_DIR_IN(void)
{
    gpio_set_pin_input(PIN_GPIO, GPIO_GET_PORT_INDEX(PIN_TMS), GPIO_GET_PIN_INDEX(PIN_TMS));
    gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(SWDIO_DIR), GPIO_GET_PIN_INDEX(SWDIO_DIR), false);
    __asm volatile("fence io, io");
}

__STATIC_FORCEINLINE void PIN_CJTAG_TMSC_DIR_OUT(void)
{
    gpio_set_pin_output(PIN_GPIO, GPIO_GET_PORT_INDEX(PIN_TMS), GPIO_GET_PIN_INDEX(PIN_TMS));
    gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(SWDIO_DIR), GPIO_GET_PIN_INDEX(SWDIO_DIR), true);
    __asm volatile("fence io, io");
}

// CJTAG/TMSC Pin I/O --------------------------------------

/** CJTAG/TMSC I/O pin: Get Input.
\return Current status of the CJTAG/TMSC DAP hardware I/O pin.
*/
__STATIC_FORCEINLINE uint32_t PIN_CJTAG_TMSC_IN(void)
{
    uint32_t sta =  gpio_read_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(PIN_TMS), GPIO_GET_PIN_INDEX(PIN_TMS));
    __asm volatile("fence io, io");
    return sta;
}

/** CJTAG/TMSC I/O pin: Set Output to High.
Set the CJTAG/TMSC DAP hardware I/O pin to high level.
*/
__STATIC_FORCEINLINE void PIN_CJTAG_TMSC_SET(void)
{
    gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(PIN_TMS), GPIO_GET_PIN_INDEX(PIN_TMS), true);
    __asm volatile("fence io, io");
}

/** CJTAG/TMSC I/O pin: Set Output to Low.
Set the CJTAG/TMSC DAP hardware I/O pin to low level.
*/
__STATIC_FORCEINLINE void PIN_CJTAG_TMSC_CLR(void)
{
    gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(PIN_TMS), GPIO_GET_PIN_INDEX(PIN_TMS), false);
    __asm volatile("fence io, io");
}

/** TMSC I/O pin: Set Output.
\param bit Output value for the TMSC DAP hardware I/O pin.
*/
__STATIC_FORCEINLINE void PIN_CJTAG_TMSC_OUT(uint32_t bit)
{
    if(bit & 0x01) {
       gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(PIN_TMS), GPIO_GET_PIN_INDEX(PIN_TMS), true);
    }
    else {
       gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(PIN_TMS), GPIO_GET_PIN_INDEX(PIN_TMS), false);
    }
    __asm volatile("fence io, io");
  }

__STATIC_FORCEINLINE void PIN_CJTAG_TMSC_OUT_TOGGLE(void)
{
    gpio_toggle_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(PIN_TMS), GPIO_GET_PIN_INDEX(PIN_TMS));
    __asm volatile("fence io, io");
}

void CJTAG_write_escape_seq(uint32_t num_toggle);
void CJTAG_write_tmsc(uint32_t data, uint32_t num_bits);
void CJTAG_zero_bit_DR_scans(void);
void CJTAG_lock_zbs(void);
void CJTAG_set_cmd_level(uint8_t level);
void CJTAG_write_TAP7_cmd_param(uint8_t cmd, uint8_t param);
void CJTAG_write_OScan1(uint32_t TMSData, uint32_t TDIData, uint32_t NumBits);
void CJTAG_standard_connect_seq(void);
void CJTAG_short_connect_seq(void);
void CJTAG_write_OScan1_cmd_param(uint8_t cmd, uint8_t param);
#endif /* CJTAG_DP_H_ */
