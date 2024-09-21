/*
 * Copyright (c) 2013-2017 ARM Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * ----------------------------------------------------------------------
 *
 * $Date:        1. December 2017
 * $Revision:    V2.0.0
 *
 * Project:      CMSIS-DAP Source
 * Title:        JTAG_DP.c CMSIS-DAP JTAG DP I/O
 *
 *---------------------------------------------------------------------------*/

#include "DAP_config.h"
#include "DAP.h"
#include "board.h"
#include "hpm_spi_drv.h"
#include "hpm_clock_drv.h"
#ifdef HPMSOC_HAS_HPMSDK_DMAV2
#include "hpm_dmav2_drv.h"
#else
#include "hpm_dma_drv.h"
#endif
#include "hpm_dmamux_drv.h"

// JTAG Macros

#define USE_GPIO_1_BIT         0

#define PIN_JTAG_GPIO          HPM_FGPIO

#define JTAG_SPI_DMA                HPM_HDMA
#define JTAG_SPI_DMAMUX             HPM_DMAMUX
#define JTAG_SPI_RX_DMA_REQ         HPM_DMA_SRC_SPI2_RX
#define JTAG_SPI_TX_DMA_REQ         HPM_DMA_SRC_SPI2_TX
#define JTAG_SPI_RX_DMA_CH          0
#define JTAG_SPI_TX_DMA_CH          1
#define JTAG_SPI_RX_DMAMUX_CH       DMA_SOC_CHN_TO_DMAMUX_CHN(JTAG_SPI_DMA, JTAG_SPI_RX_DMA_CH)
#define JTAG_SPI_TX_DMAMUX_CH       DMA_SOC_CHN_TO_DMAMUX_CHN(JTAG_SPI_DMA, JTAG_SPI_TX_DMA_CH)
#define JTAG_SPI_SCLK_FREQ         (20000000UL)

#define PIN_SINGLE_SPI_JTAG_TMS    IOC_PAD_PA29
#define PIN_JTAG_TCK               IOC_PAD_PB11
#define PIN_JTAG_TDO               IOC_PAD_PB12
#define PIN_JTAG_TDI               IOC_PAD_PB13

#define PIN_JTAG_DELAY() PIN_DELAY_SLOW(DAP_Data.clock_delay)

static void jtag_spi_sequece (uint32_t info, const uint8_t *tdi, uint8_t *tdo);
static void jtag_spi_ir_fast(uint32_t ir, uint16_t ir_before, uint8_t ir_length, uint16_t ir_after);
static uint8_t jtag_spi_transfer_fast(uint32_t request, uint32_t *data, uint8_t index_count, uint8_t index, uint8_t idle_cycles);
static uint32_t jtag_spi_read_idcode(uint8_t index);
static void jtag_spi_write_abort(uint32_t data, uint8_t index_count, uint8_t index);
static void jtag_emulation_init(void);

#if (DAP_JTAG != 0)


void PORT_JTAG_SETUP(void)
{
    clock_add_to_group(JTAG_SPI_BASE_CLOCK_NAME, 0);
    clock_set_source_divider(JTAG_SPI_BASE_CLOCK_NAME, clk_src_pll1_clk0, 10U); /* 80MHz */
    HPM_IOC->PAD[IOC_PAD_PB10].FUNC_CTL = IOC_PB10_FUNC_CTL_GPIO_B_10;
    HPM_IOC->PAD[PIN_JTAG_TCK].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(5) | IOC_PAD_FUNC_CTL_LOOP_BACK_SET(1); /* as spi sck*/
    HPM_IOC->PAD[PIN_JTAG_TDO].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(5); /* as spi mosi */
    HPM_IOC->PAD[PIN_JTAG_TDI].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(5); /** as spi miso */
    HPM_IOC->PAD[PIN_SINGLE_SPI_JTAG_TMS].FUNC_CTL = IOC_PA29_FUNC_CTL_GPIO_A_29;
    HPM_IOC->PAD[IOC_PAD_PB10].PAD_CTL = IOC_PAD_PAD_CTL_SR_MASK | IOC_PAD_PAD_CTL_SPD_SET(3);
    HPM_IOC->PAD[PIN_JTAG_TCK].PAD_CTL = IOC_PAD_PAD_CTL_SR_MASK | IOC_PAD_PAD_CTL_SPD_SET(3);
    HPM_IOC->PAD[PIN_JTAG_TDO].PAD_CTL = IOC_PAD_PAD_CTL_SR_MASK | IOC_PAD_PAD_CTL_SPD_SET(3);
    HPM_IOC->PAD[PIN_JTAG_TDI].PAD_CTL = IOC_PAD_PAD_CTL_SR_MASK | IOC_PAD_PAD_CTL_SPD_SET(3) | IOC_PAD_PAD_CTL_PE_SET(1) | IOC_PAD_PAD_CTL_PS_SET(0);
    HPM_IOC->PAD[PIN_SINGLE_SPI_JTAG_TMS].PAD_CTL = IOC_PAD_PAD_CTL_SR_MASK | IOC_PAD_PAD_CTL_SPD_SET(3);
    gpiom_configure_pin_control_setting(PIN_SINGLE_SPI_JTAG_TMS);
    gpio_set_pin_output(PIN_JTAG_GPIO, GPIO_GET_PORT_INDEX(PIN_SINGLE_SPI_JTAG_TMS), GPIO_GET_PIN_INDEX(PIN_SINGLE_SPI_JTAG_TMS));
#if defined(USE_GPIO_1_BIT) && (USE_GPIO_1_BIT == 1)
    gpiom_configure_pin_control_setting(PIN_JTAG_TCK);
    gpiom_configure_pin_control_setting(PIN_JTAG_TDO);
    gpiom_configure_pin_control_setting(PIN_JTAG_TDI);
    gpiom_configure_pin_control_setting(IOC_PAD_PB10);
    gpio_set_pin_output(PIN_JTAG_GPIO, GPIO_GET_PORT_INDEX(IOC_PAD_PB10), GPIO_GET_PIN_INDEX(IOC_PAD_PB10));
    gpio_set_pin_output(PIN_JTAG_GPIO, GPIO_GET_PORT_INDEX(PIN_JTAG_TCK), GPIO_GET_PIN_INDEX(PIN_JTAG_TCK));
    gpio_set_pin_output(PIN_JTAG_GPIO, GPIO_GET_PORT_INDEX(PIN_JTAG_TDI), GPIO_GET_PIN_INDEX(PIN_JTAG_TDI));
    gpio_set_pin_input(PIN_JTAG_GPIO, GPIO_GET_PORT_INDEX(PIN_JTAG_TDO), GPIO_GET_PIN_INDEX(PIN_JTAG_TDO));
#endif
    jtag_emulation_init();
}

// Generate JTAG Sequence
//   info:   sequence information
//   tdi:    pointer to TDI generated data
//   tdo:    pointer to TDO captured data
//   return: none
void JTAG_Sequence (uint32_t info, const uint8_t *tdi, uint8_t *tdo) {
    jtag_spi_sequece(info, tdi, tdo);
}

static void JTAG_IR_Fast(uint32_t ir)
{
    uint16_t ir_before, ir_after;
    uint8_t ir_length;
    ir_before = DAP_Data.jtag_dev.ir_before[DAP_Data.jtag_dev.index];
    ir_length = DAP_Data.jtag_dev.ir_length[DAP_Data.jtag_dev.index];
    ir_after = DAP_Data.jtag_dev.ir_after[DAP_Data.jtag_dev.index];
    jtag_spi_ir_fast(ir, ir_before, ir_length, ir_after);
}

static void JTAG_IR_Slow(uint32_t ir)
{
    JTAG_IR_Fast(ir);
}

static uint8_t JTAG_TransferFast(uint32_t request, uint32_t *data)
{
    uint8_t index_count, index, idle_cycles;
    index_count = DAP_Data.jtag_dev.count;
    index = DAP_Data.jtag_dev.index;
    idle_cycles = DAP_Data.transfer.idle_cycles;
    return jtag_spi_transfer_fast(request, data, index_count, index, idle_cycles);
}

static uint8_t JTAG_TransferSlow(uint32_t request, uint32_t *data)
{
    return JTAG_TransferFast(request, data);
}

// JTAG Read IDCODE register
//   return: value read
uint32_t JTAG_ReadIDCode (void) {
    uint8_t index = DAP_Data.jtag_dev.index;
    return jtag_spi_read_idcode(index);
}


// JTAG Write ABORT register
//   data:   value to write
//   return: none
void JTAG_WriteAbort (uint32_t data) {
    uint8_t index_count, index;
    index_count = DAP_Data.jtag_dev.count;
    index = DAP_Data.jtag_dev.index;
    jtag_spi_write_abort(data, index_count, index);
}


// JTAG Set IR
//   ir:     IR value
//   return: none
void JTAG_IR (uint32_t ir) {
  if (DAP_Data.fast_clock) {
    JTAG_IR_Fast(ir);
  } else {
    JTAG_IR_Slow(ir);
  }
}


// JTAG Transfer I/O
//   request: A[3:2] RnW APnDP
//   data:    DATA[31:0]
//   return:  ACK[2:0]
uint8_t  JTAG_Transfer(uint32_t request, uint32_t *data) {
  if (DAP_Data.fast_clock) {
    return JTAG_TransferFast(request, data);
  } else {
    return JTAG_TransferSlow(request, data);
  }
}


static void jtag_reset(void)
{
    JTAG_SPI_BASE->CTRL |= SPI_CTRL_RXFIFORST_MASK | SPI_CTRL_TXFIFORST_MASK;
    while (JTAG_SPI_BASE->STATUS & (SPI_CTRL_RXFIFORST_MASK | SPI_CTRL_TXFIFORST_MASK)) {
    };
}

static void jtag_less_than_32bit_size_for_tck(uint16_t bit_size, uint32_t dummy)
{
    if (bit_size == 0)
        return;
    jtag_reset();
    spi_set_transfer_mode(JTAG_SPI_BASE, spi_trans_write_only);
    spi_set_data_bits(JTAG_SPI_BASE, bit_size);
    spi_set_read_data_count(JTAG_SPI_BASE, 1);
    spi_set_write_data_count(JTAG_SPI_BASE, 1);
    JTAG_SPI_BASE->CMD = 0xFF; /* Write a dummy byte */
    JTAG_SPI_BASE->DATA = dummy;
    while (JTAG_SPI_BASE->STATUS & SPI_STATUS_SPIACTIVE_MASK) {
    };
}

static void jtag_more_than_32bit_size_for_tck(uint16_t bit_size, uint32_t dummy)
{
    uint32_t n_len_in_byte = 0;
    uint32_t n_len_remain_bit = 0;
    uint32_t i;

    if (bit_size == 0) {
        return;
    }
    n_len_in_byte = bit_size / 32;
    n_len_remain_bit = bit_size % 32;
    if (n_len_in_byte) {
        jtag_reset();
        spi_set_transfer_mode(JTAG_SPI_BASE, spi_trans_write_only);
        spi_set_data_bits(JTAG_SPI_BASE, 32);
        spi_set_read_data_count(JTAG_SPI_BASE, n_len_in_byte);
        spi_set_write_data_count(JTAG_SPI_BASE, n_len_in_byte);
        for (i = 0; i < n_len_in_byte; i++) {
            JTAG_SPI_BASE->DATA = dummy;
            while ((JTAG_SPI_BASE->STATUS & SPI_STATUS_TXFULL_MASK) == SPI_STATUS_TXFULL_MASK) {
            };
        }
        while (JTAG_SPI_BASE->STATUS & SPI_STATUS_SPIACTIVE_MASK) {
        };
    }
    if (n_len_remain_bit) {
        jtag_less_than_32bit_size_for_tck(n_len_remain_bit, dummy);
    }
}

// Generate JTAG Sequence
//   info:   sequence information
//   tdi:    pointer to TDI generated data
//   tdo:    pointer to TDO captured data
//   return: none
static void jtag_spi_sequece (uint32_t info, const uint8_t *tdi, uint8_t *tdo)
{
    uint32_t n;
    uint32_t n_len_in_byte = 0;
    uint32_t n_len_remain_bit = 0;
    uint32_t rx_index = 0, tx_index = 0;
    bool is_tdo = false;
    uint8_t txfifo_size = spi_get_tx_fifo_size(JTAG_SPI_BASE);
    uint8_t txfifo_valid_size = 0, rxfifo_valid_size = 0, j = 0;
    n = info & JTAG_SEQUENCE_TCK;
    if (info & JTAG_SEQUENCE_TMS) {
         //printf("1s: %d %d\n", n_len_in_byte, n_len_remain_bit);
          gpio_write_pin(PIN_JTAG_GPIO, GPIO_GET_PORT_INDEX(PIN_SINGLE_SPI_JTAG_TMS), GPIO_GET_PIN_INDEX(PIN_SINGLE_SPI_JTAG_TMS), true);
    } else {
         //printf("0s: %d %d\n", n_len_in_byte, n_len_remain_bit);
         gpio_write_pin(PIN_JTAG_GPIO, GPIO_GET_PORT_INDEX(PIN_SINGLE_SPI_JTAG_TMS), GPIO_GET_PIN_INDEX(PIN_SINGLE_SPI_JTAG_TMS), false);
    }
    if (n == 1) {
#if defined(USE_GPIO_1_BIT) && (USE_GPIO_1_BIT == 1)

        HPM_IOC->PAD[PIN_JTAG_TCK].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);
        HPM_IOC->PAD[PIN_JTAG_TDI].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);
        PIN_JTAG_DELAY();
        if((*tdi) & 0x01) {
            gpio_write_pin(PIN_JTAG_GPIO, GPIO_GET_PORT_INDEX(PIN_JTAG_TDI), GPIO_GET_PIN_INDEX(PIN_JTAG_TDI), true);
        }
        else {
            gpio_write_pin(PIN_JTAG_GPIO, GPIO_GET_PORT_INDEX(PIN_JTAG_TDI), GPIO_GET_PIN_INDEX(PIN_JTAG_TDI), false);
        }
        gpio_write_pin(PIN_JTAG_GPIO, GPIO_GET_PORT_INDEX(PIN_JTAG_TCK), GPIO_GET_PIN_INDEX(PIN_JTAG_TCK), true);
        // PIN_JTAG_DELAY();
        if (info & JTAG_SEQUENCE_TDO) {
            (*tdo) = gpio_read_pin(PIN_JTAG_GPIO, GPIO_GET_PORT_INDEX(PIN_JTAG_TDO), GPIO_GET_PIN_INDEX(PIN_JTAG_TDO));
        }
        gpio_write_pin(PIN_JTAG_GPIO, GPIO_GET_PORT_INDEX(PIN_JTAG_TCK), GPIO_GET_PIN_INDEX(PIN_JTAG_TCK), false);
        // PIN_JTAG_DELAY();
        HPM_IOC->PAD[PIN_JTAG_TCK].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(5) | IOC_PAD_FUNC_CTL_LOOP_BACK_SET(1); 
        HPM_IOC->PAD[PIN_JTAG_TDO].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(5);
        HPM_IOC->PAD[PIN_JTAG_TDI].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(5);
        return;
#else
        switch ((*tdi) & 0x01)
        {
        case 1:
            JTAG_SPI_BASE->DIRECTIO = 0x1040400;
            JTAG_SPI_BASE->DIRECTIO = 0x1060600; /* diretion = 1, slck_oe = 1, mosi_oe = 1, sclk_o = 1, mosi_o = 1 */
            if (is_tdo == true) {
                *(uint8_t *)(tdo) = (uint8_t)(JTAG_SPI_BASE->DIRECTIO >> 3);
            }
            JTAG_SPI_BASE->DIRECTIO = 0x1060400; /* diretion = 1, slck_oe = 1, mosi_oe = 1, sclk_o = 0, mosi_o = 1*/
            break;
        case 0:
            JTAG_SPI_BASE->DIRECTIO = 0x1040000;
            JTAG_SPI_BASE->DIRECTIO = 0x1060200; /* diretion = 1, slck_oe = 1, mosi_oe = 1, sclk_o = 1, mosi_o = 0 */
            if (is_tdo == true) {
                *(uint8_t *)(tdo) = (uint8_t)(JTAG_SPI_BASE->DIRECTIO >> 3);
            }
            JTAG_SPI_BASE->DIRECTIO = 0x1060000; /* diretion = 1, slck_oe = 1, mosi_oe = 1, sclk_o = 0, mosi_o = 0 */
        default:
            break;
        }
        JTAG_SPI_BASE->DIRECTIO = 0; /* diretion = 0 */
        return;
#endif
    }

    jtag_reset();
    if (info & JTAG_SEQUENCE_TDO) {
        is_tdo = true;
        spi_set_transfer_mode(JTAG_SPI_BASE, spi_trans_write_read_together);
    } else {
        spi_set_transfer_mode(JTAG_SPI_BASE, spi_trans_write_only);
    }
    if (n == 0U) {
        n = 64U;
    }
    n_len_in_byte = n / 8;
    n_len_remain_bit = n % 8;

    if (n_len_in_byte) {
        spi_set_data_bits(JTAG_SPI_BASE, 8);
        spi_set_read_data_count(JTAG_SPI_BASE, n_len_in_byte);
        spi_set_write_data_count(JTAG_SPI_BASE, n_len_in_byte);
        JTAG_SPI_BASE->CMD = 0xFF; /* Write a dummy byte */
        if (is_tdo == false) {
            rx_index = n_len_in_byte;
        }
        while ((rx_index < n_len_in_byte) || (tx_index < n_len_in_byte)) {
            if (tx_index < n_len_in_byte) {
                txfifo_valid_size = spi_get_tx_fifo_valid_data_size(JTAG_SPI_BASE);
                if ((txfifo_size - txfifo_valid_size) > 0) {
                    for (j = 0; j < (txfifo_size - txfifo_valid_size); j++) {
                        if (tx_index >= n_len_in_byte) {
                            break;
                        }
                        JTAG_SPI_BASE->DATA = *tdi;
                        tdi++;
                        tx_index++;
                    }
                }
            }
            if (rx_index < n_len_in_byte) {
                rxfifo_valid_size = spi_get_rx_fifo_valid_data_size(JTAG_SPI_BASE);
                if (rxfifo_valid_size > 0) {
                    for (j = 0; j < rxfifo_valid_size; j++) {
                        if (rx_index >= n_len_in_byte) {
                            break;
                        }
                        *tdo = JTAG_SPI_BASE->DATA;
                        tdo++;
                        rx_index++;
                    }
                }
            }
        }
        while (JTAG_SPI_BASE->STATUS & SPI_STATUS_SPIACTIVE_MASK) {
        };
    }
    if (n_len_remain_bit) {
        spi_set_data_bits(JTAG_SPI_BASE, n_len_remain_bit);
        spi_set_read_data_count(JTAG_SPI_BASE, 1);
        spi_set_write_data_count(JTAG_SPI_BASE, 1);
        JTAG_SPI_BASE->CMD = 0xFF; /* Write a dummy byte */
        JTAG_SPI_BASE->DATA = *(uint8_t *)(tdi);
        if (is_tdo == true) {
            while ((JTAG_SPI_BASE->STATUS & SPI_STATUS_RXEMPTY_MASK) == SPI_STATUS_RXEMPTY_MASK) {
            };
            *(uint8_t *)(tdo) = JTAG_SPI_BASE->DATA;
        }
        while (JTAG_SPI_BASE->STATUS & SPI_STATUS_SPIACTIVE_MASK) {
        };
    }
}

static void jtag_ir(uint32_t ir, uint8_t ir_bit_size)
{
    uint8_t tmp_bits = 0;
    tmp_bits = (ir_bit_size > 32) ? 32 : ir_bit_size;
    jtag_less_than_32bit_size_for_tck(tmp_bits, ir);
    ir_bit_size -= tmp_bits;
    if (ir_bit_size) {
        jtag_more_than_32bit_size_for_tck(ir_bit_size, 0x0);
    }
}

static void jtag_spi_ir_fast(uint32_t ir, uint16_t ir_before, uint8_t ir_length, uint16_t ir_after)
{
    spi_set_transfer_mode(JTAG_SPI_BASE, spi_trans_write_only);
    /* Select-DR-Scan  and Select-IR-Scan */
    gpio_write_pin(PIN_JTAG_GPIO, GPIO_GET_PORT_INDEX(PIN_SINGLE_SPI_JTAG_TMS), GPIO_GET_PIN_INDEX(PIN_SINGLE_SPI_JTAG_TMS), true);
    jtag_less_than_32bit_size_for_tck(2, 0);
      /* Capture-IR   and Shift-IR */
    gpio_write_pin(PIN_JTAG_GPIO, GPIO_GET_PORT_INDEX(PIN_SINGLE_SPI_JTAG_TMS), GPIO_GET_PIN_INDEX(PIN_SINGLE_SPI_JTAG_TMS), false);
    jtag_less_than_32bit_size_for_tck(2, 0);
    if (ir_before) {
        if (ir_before > 32) {
            jtag_more_than_32bit_size_for_tck(ir_before, 0xFFFFFFFF); /* Bypass before data */
        } else {
            jtag_less_than_32bit_size_for_tck(ir_before, 0xFFFFFFFF);
        }
    }
    if (ir_length) {
        jtag_ir(ir, ir_length - 1); /* Set IR bits (except last) */
        ir >>= (ir_length - 1);
    }
    if (ir_after) {
        /* Set last IR bit */
        jtag_less_than_32bit_size_for_tck(1, ir);
        ir_after -= 1;
        if (ir_after > 32) {
            jtag_more_than_32bit_size_for_tck(ir_after, 0xFFFFFFFF); /* Bypass after data */
        } else {
            jtag_less_than_32bit_size_for_tck(ir_after, 0xFFFFFFFF);
        }
        /* Bypass & Exit1-IR */
        gpio_write_pin(PIN_JTAG_GPIO, GPIO_GET_PORT_INDEX(PIN_SINGLE_SPI_JTAG_TMS), GPIO_GET_PIN_INDEX(PIN_SINGLE_SPI_JTAG_TMS), true);
        jtag_less_than_32bit_size_for_tck(1, 0xFFFFFFFF);
    } else {
        /* Set last IR bit & Exit1-IR  */
        gpio_write_pin(PIN_JTAG_GPIO, GPIO_GET_PORT_INDEX(PIN_SINGLE_SPI_JTAG_TMS), GPIO_GET_PIN_INDEX(PIN_SINGLE_SPI_JTAG_TMS), true);
        jtag_less_than_32bit_size_for_tck(1, ir);
    }
    /* Update-IR */
    jtag_less_than_32bit_size_for_tck(1, 0);
    /* Idle */
    gpio_write_pin(PIN_JTAG_GPIO, GPIO_GET_PORT_INDEX(PIN_SINGLE_SPI_JTAG_TMS), GPIO_GET_PIN_INDEX(PIN_SINGLE_SPI_JTAG_TMS), false);
    jtag_less_than_32bit_size_for_tck(1, 0xFFFFFFFF);
}

static uint8_t jtag_spi_transfer_fast(uint32_t request, uint32_t *data, uint8_t index_count, uint8_t index, uint8_t idle_cycles)
{
    uint32_t ack = 0;
    uint32_t bit = 0;
    uint32_t n = 0;
    uint32_t val;
    /* Select-DR-Scan */
    gpio_write_pin(PIN_JTAG_GPIO, GPIO_GET_PORT_INDEX(PIN_SINGLE_SPI_JTAG_TMS), GPIO_GET_PIN_INDEX(PIN_SINGLE_SPI_JTAG_TMS), true);
    jtag_less_than_32bit_size_for_tck(1, 0);
    /* Capture-DR & Shift-DR */  
    gpio_write_pin(PIN_JTAG_GPIO, GPIO_GET_PORT_INDEX(PIN_SINGLE_SPI_JTAG_TMS), GPIO_GET_PIN_INDEX(PIN_SINGLE_SPI_JTAG_TMS), false);
    jtag_less_than_32bit_size_for_tck(2, 0);
     /* Bypass before data */
    if (index) {
        if (index > 32) {
            jtag_more_than_32bit_size_for_tck(index, 0xFFFFFFFF); /* Bypass before data */
        } else {
            jtag_less_than_32bit_size_for_tck(index, 0xFFFFFFFF);
        }
    }
    jtag_reset();
    /* Set RnW A2 A3, Get ACK.0 ACK.1 ACK.2*/
    spi_set_transfer_mode(JTAG_SPI_BASE, spi_trans_write_read_together);
    spi_set_data_bits(JTAG_SPI_BASE, 3);
    spi_set_read_data_count(JTAG_SPI_BASE, 1);
    spi_set_write_data_count(JTAG_SPI_BASE, 1);
    JTAG_SPI_BASE->CMD = 0xFF; /* Write a dummy byte */
    JTAG_SPI_BASE->DATA = request >> 0x01;
    while ((JTAG_SPI_BASE->STATUS & SPI_STATUS_RXEMPTY_MASK) == SPI_STATUS_RXEMPTY_MASK) {
    };
    bit = JTAG_SPI_BASE->DATA;
    while (JTAG_SPI_BASE->STATUS & SPI_STATUS_SPIACTIVE_MASK) {
    };
    ack = ((bit & 0x01) << 1) | ((bit & 0x02) >> 1) | (bit & 0x04);
    if (ack != DAP_TRANSFER_OK) {
        gpio_write_pin(PIN_JTAG_GPIO, GPIO_GET_PORT_INDEX(PIN_SINGLE_SPI_JTAG_TMS), GPIO_GET_PIN_INDEX(PIN_SINGLE_SPI_JTAG_TMS), true);
        jtag_less_than_32bit_size_for_tck(1, 0xFFFFFFFF);
        goto exit;
    }
    jtag_reset();
    if (request & DAP_TRANSFER_RnW) {
        /* Read Transfer */
        val = 0;
        /* Get D0..D30 */
        spi_set_transfer_mode(JTAG_SPI_BASE, spi_trans_read_only);
        spi_set_data_bits(JTAG_SPI_BASE, 31);
        spi_set_read_data_count(JTAG_SPI_BASE, 1);
        spi_set_write_data_count(JTAG_SPI_BASE, 1);
        JTAG_SPI_BASE->CMD = 0xFF; /* Write a dummy byte */
        while ((JTAG_SPI_BASE->STATUS & SPI_STATUS_RXEMPTY_MASK) == SPI_STATUS_RXEMPTY_MASK) {
        };
        bit = JTAG_SPI_BASE->DATA;
        while (JTAG_SPI_BASE->STATUS & SPI_STATUS_SPIACTIVE_MASK) {
        };
        val = bit;
        n = index_count - index - 1U;
        bit = 0;
        jtag_reset();
        spi_set_data_bits(JTAG_SPI_BASE, 1);
        spi_set_read_data_count(JTAG_SPI_BASE, 1);
        spi_set_write_data_count(JTAG_SPI_BASE, 1);
        if (n) {
            /* Get D31 */ 
            JTAG_SPI_BASE->CMD = 0xFF; /* Write a dummy byte */
            while ((JTAG_SPI_BASE->STATUS & SPI_STATUS_RXEMPTY_MASK) == SPI_STATUS_RXEMPTY_MASK) {
            };
            bit = JTAG_SPI_BASE->DATA;
            while (JTAG_SPI_BASE->STATUS & SPI_STATUS_SPIACTIVE_MASK) {
            };
            /* Bypass after data */
            jtag_less_than_32bit_size_for_tck(n - 1, 0xFFFFFFFF);
            /* Bypass & Exit1-DR */
            gpio_write_pin(PIN_JTAG_GPIO, GPIO_GET_PORT_INDEX(PIN_SINGLE_SPI_JTAG_TMS), GPIO_GET_PIN_INDEX(PIN_SINGLE_SPI_JTAG_TMS), true);
            jtag_less_than_32bit_size_for_tck(1, 0xFFFFFFFF);
        } else {
            /* Get D31 & Exit1-DR */
            gpio_write_pin(PIN_JTAG_GPIO, GPIO_GET_PORT_INDEX(PIN_SINGLE_SPI_JTAG_TMS), GPIO_GET_PIN_INDEX(PIN_SINGLE_SPI_JTAG_TMS), true);
            JTAG_SPI_BASE->CMD = 0xFF; /* Write a dummy byte */
            while ((JTAG_SPI_BASE->STATUS & SPI_STATUS_RXEMPTY_MASK) == SPI_STATUS_RXEMPTY_MASK) {
            };
            bit = JTAG_SPI_BASE->DATA;
            while (JTAG_SPI_BASE->STATUS & SPI_STATUS_SPIACTIVE_MASK) {
            };
        }
        val |= bit << 31;
        if (data) {
            *data = val;
        }
    } else {
        /* Write Transfer */ 
        val = (*data);
        spi_set_transfer_mode(JTAG_SPI_BASE, spi_trans_write_only);
        /* Set D0..D30 */
        jtag_less_than_32bit_size_for_tck(31, val);
        n = index_count - index - 1U;
        val >>= 31;
        spi_set_data_bits(JTAG_SPI_BASE, 1);
        spi_set_read_data_count(JTAG_SPI_BASE, 1);
        spi_set_write_data_count(JTAG_SPI_BASE, 1);
        if (n) {
            JTAG_SPI_BASE->CMD = 0xFF; /* Write a dummy byte */
            JTAG_SPI_BASE->DATA = val;
            while (JTAG_SPI_BASE->STATUS & SPI_STATUS_SPIACTIVE_MASK) {
            };
            /* Bypass after data */
            jtag_less_than_32bit_size_for_tck(n - 1, 0);
            /* Bypass & Exit1-DR */
            gpio_write_pin(PIN_JTAG_GPIO, GPIO_GET_PORT_INDEX(PIN_SINGLE_SPI_JTAG_TMS), GPIO_GET_PIN_INDEX(PIN_SINGLE_SPI_JTAG_TMS), true);
            jtag_less_than_32bit_size_for_tck(1, 0);
        } else {
            gpio_write_pin(PIN_JTAG_GPIO, GPIO_GET_PORT_INDEX(PIN_SINGLE_SPI_JTAG_TMS), GPIO_GET_PIN_INDEX(PIN_SINGLE_SPI_JTAG_TMS), true);
            JTAG_SPI_BASE->CMD = 0xFF; /* Write a dummy byte */
            JTAG_SPI_BASE->DATA = val;
            while (JTAG_SPI_BASE->STATUS & SPI_STATUS_SPIACTIVE_MASK) {
            };
        }
    }
exit:
    gpio_write_pin(PIN_JTAG_GPIO, GPIO_GET_PORT_INDEX(PIN_SINGLE_SPI_JTAG_TMS), GPIO_GET_PIN_INDEX(PIN_SINGLE_SPI_JTAG_TMS), 1);
    /* Update-DR */ 
    jtag_less_than_32bit_size_for_tck(1, 0);
    board_write_spi_cs(BOARD_SPI_CS_PIN, false);
    gpio_write_pin(PIN_JTAG_GPIO, GPIO_GET_PORT_INDEX(PIN_SINGLE_SPI_JTAG_TMS), GPIO_GET_PIN_INDEX(PIN_SINGLE_SPI_JTAG_TMS), 0);
    /* Idle */
    jtag_less_than_32bit_size_for_tck(1, 0xFFFFFFFF);

    /* Capture Timestamp */
    if (request & DAP_TRANSFER_TIMESTAMP) {
        // DAP_Data.timestamp = TIMESTAMP_GET(); 
    }

    gpio_write_pin(PIN_JTAG_GPIO, GPIO_GET_PORT_INDEX(PIN_SINGLE_SPI_JTAG_TMS), GPIO_GET_PIN_INDEX(PIN_SINGLE_SPI_JTAG_TMS), 0);
    /* Idle cycles */ 
    jtag_less_than_32bit_size_for_tck(idle_cycles, 0xFFFFFFFF);
    return ((uint8_t)ack);
}

// JTAG Read IDCODE register
//   return: value read
static uint32_t jtag_spi_read_idcode (uint8_t index)
{
    uint32_t bit;
    uint32_t val;
    /* Select-DR-Scan */
    gpio_write_pin(PIN_JTAG_GPIO, GPIO_GET_PORT_INDEX(PIN_SINGLE_SPI_JTAG_TMS), GPIO_GET_PIN_INDEX(PIN_SINGLE_SPI_JTAG_TMS), true);
    jtag_less_than_32bit_size_for_tck(1, 0);

    /* Capture-DR & Shift-DR */  
    gpio_write_pin(PIN_JTAG_GPIO, GPIO_GET_PORT_INDEX(PIN_SINGLE_SPI_JTAG_TMS), GPIO_GET_PIN_INDEX(PIN_SINGLE_SPI_JTAG_TMS), false);
    jtag_less_than_32bit_size_for_tck(2, 0);

    /* Bypass before data */
    jtag_more_than_32bit_size_for_tck(index, 0xFFFFFFFF);

    val = 0U;
    /* Get D0..D30 */
    jtag_reset();
    spi_set_transfer_mode(JTAG_SPI_BASE, spi_trans_read_only);
    spi_set_data_bits(JTAG_SPI_BASE, 32);
    spi_set_read_data_count(JTAG_SPI_BASE, 1);
    spi_set_write_data_count(JTAG_SPI_BASE, 1);
    JTAG_SPI_BASE->CMD = 0xFF; /* Write a dummy byte */
    while ((JTAG_SPI_BASE->STATUS & SPI_STATUS_RXEMPTY_MASK) == SPI_STATUS_RXEMPTY_MASK) {
    };
    bit = JTAG_SPI_BASE->DATA;
    while (JTAG_SPI_BASE->STATUS & SPI_STATUS_SPIACTIVE_MASK) {
    };
    val = bit;
    bit = 0;
    jtag_reset();
    spi_set_data_bits(JTAG_SPI_BASE, 1);
    spi_set_read_data_count(JTAG_SPI_BASE, 1);
    spi_set_write_data_count(JTAG_SPI_BASE, 1);
    /* Get D31 & Exit1-DR */
    gpio_write_pin(PIN_JTAG_GPIO, GPIO_GET_PORT_INDEX(PIN_SINGLE_SPI_JTAG_TMS), GPIO_GET_PIN_INDEX(PIN_SINGLE_SPI_JTAG_TMS), true);
    JTAG_SPI_BASE->CMD = 0xFF; /* Write a dummy byte */
    while ((JTAG_SPI_BASE->STATUS & SPI_STATUS_RXEMPTY_MASK) == SPI_STATUS_RXEMPTY_MASK) {
    };
    bit = JTAG_SPI_BASE->DATA;
    while (JTAG_SPI_BASE->STATUS & SPI_STATUS_SPIACTIVE_MASK) {
    };
    val |= bit << 31;
    gpio_write_pin(PIN_JTAG_GPIO, GPIO_GET_PORT_INDEX(PIN_SINGLE_SPI_JTAG_TMS), GPIO_GET_PIN_INDEX(PIN_SINGLE_SPI_JTAG_TMS), 1);
    /* Update-DR */ 
    jtag_less_than_32bit_size_for_tck(1, 0);
    board_write_spi_cs(BOARD_SPI_CS_PIN, false);
    gpio_write_pin(PIN_JTAG_GPIO, GPIO_GET_PORT_INDEX(PIN_SINGLE_SPI_JTAG_TMS), GPIO_GET_PIN_INDEX(PIN_SINGLE_SPI_JTAG_TMS), 0);
    /* Idle */
    jtag_less_than_32bit_size_for_tck(1, 0xFFFFFFFF);
    return (val);
}

// JTAG Write ABORT register
//   data:   value to write
//   return: none
static void jtag_spi_write_abort (uint32_t data, uint8_t index_count, uint8_t index)
{
    uint32_t n = 0;
    /* Select-DR-Scan */
    gpio_write_pin(PIN_JTAG_GPIO, GPIO_GET_PORT_INDEX(PIN_SINGLE_SPI_JTAG_TMS), GPIO_GET_PIN_INDEX(PIN_SINGLE_SPI_JTAG_TMS), true);
    jtag_less_than_32bit_size_for_tck(1, 0);

    /* Capture-DR & Shift-DR */  
     gpio_write_pin(PIN_JTAG_GPIO, GPIO_GET_PORT_INDEX(PIN_SINGLE_SPI_JTAG_TMS), GPIO_GET_PIN_INDEX(PIN_SINGLE_SPI_JTAG_TMS), false);
    jtag_less_than_32bit_size_for_tck(2, 0);

    /* Bypass before data */
    jtag_more_than_32bit_size_for_tck(index, 0xFFFFFFFF);

    /* Set RnW=0 (Write) & Set A2 = 0 & Set A3 = 0 */
    jtag_reset();
    spi_set_transfer_mode(JTAG_SPI_BASE, spi_trans_write_only);
    spi_set_data_bits(JTAG_SPI_BASE, 3);
    spi_set_read_data_count(JTAG_SPI_BASE, 1);
    spi_set_write_data_count(JTAG_SPI_BASE, 1);
    JTAG_SPI_BASE->CMD = 0xFF; /* Write a dummy byte */
    JTAG_SPI_BASE->DATA = n;
    while (JTAG_SPI_BASE->STATUS & SPI_STATUS_SPIACTIVE_MASK) {
    };

    /* Set D0..D30 */
    jtag_less_than_32bit_size_for_tck(31, data);
    n = index_count - index - 1U;
    /* Set D31 */
    data >>= 31;
    jtag_reset();
    spi_set_transfer_mode(JTAG_SPI_BASE, spi_trans_write_only);
    spi_set_data_bits(JTAG_SPI_BASE, 1);
    spi_set_read_data_count(JTAG_SPI_BASE, 1);
    spi_set_write_data_count(JTAG_SPI_BASE, 1);
    if (n) {
         /* Bypass after data */
        JTAG_SPI_BASE->CMD = 0xFF; /* Write a dummy byte */
        JTAG_SPI_BASE->DATA = data;
        while (JTAG_SPI_BASE->STATUS & SPI_STATUS_SPIACTIVE_MASK) {
        };
        /* Bypass after data */
        jtag_less_than_32bit_size_for_tck(n - 1, 0);
        /* Bypass & Exit1-DR */
        gpio_write_pin(PIN_JTAG_GPIO, GPIO_GET_PORT_INDEX(PIN_SINGLE_SPI_JTAG_TMS), GPIO_GET_PIN_INDEX(PIN_SINGLE_SPI_JTAG_TMS), true);
        jtag_less_than_32bit_size_for_tck(1, 0);
    } else {
        /* Set D31 & Exit1-DR */
        gpio_write_pin(PIN_JTAG_GPIO, GPIO_GET_PORT_INDEX(PIN_SINGLE_SPI_JTAG_TMS), GPIO_GET_PIN_INDEX(PIN_SINGLE_SPI_JTAG_TMS), true);
        JTAG_SPI_BASE->CMD = 0xFF; /* Write a dummy byte */
        JTAG_SPI_BASE->DATA = data;
        while (JTAG_SPI_BASE->STATUS & SPI_STATUS_SPIACTIVE_MASK) {
        };
    }
    /* Update-DR */ 
    jtag_less_than_32bit_size_for_tck(1, 0);
    gpio_write_pin(PIN_JTAG_GPIO, GPIO_GET_PORT_INDEX(PIN_SINGLE_SPI_JTAG_TMS), GPIO_GET_PIN_INDEX(PIN_SINGLE_SPI_JTAG_TMS), false);
    /* Idle */
    jtag_less_than_32bit_size_for_tck(1, 0xFFFFFFFF);
}

static void jtag_emulation_init(void)
{
    spi_timing_config_t timing_config = {0};
    spi_format_config_t format_config = {0};
    spi_control_config_t control_config = {0};
    uint32_t spi_clcok;
    uint32_t pll_clk = 0, div = 0;
    /* set SPI sclk frequency for master */
    spi_clcok = board_init_spi_clock(JTAG_SPI_BASE);
    spi_master_get_default_timing_config(&timing_config);
    timing_config.master_config.cs2sclk = spi_cs2sclk_half_sclk_1;
    timing_config.master_config.csht = spi_csht_half_sclk_1;
    timing_config.master_config.clk_src_freq_in_hz = spi_clcok;
    timing_config.master_config.sclk_freq_in_hz = JTAG_SPI_SCLK_FREQ;
    if (status_success != spi_master_timing_init(JTAG_SPI_BASE, &timing_config)) {
        spi_master_set_sclk_div(JTAG_SPI_BASE, 0xFF);
        pll_clk = get_frequency_for_source(clock_source_pll1_clk0);
        div = pll_clk / JTAG_SPI_SCLK_FREQ;
        clock_set_source_divider(JTAG_SPI_BASE_CLOCK_NAME, clk_src_pll1_clk0, div);
    }

    /* set SPI format config for master */
    spi_master_get_default_format_config(&format_config);
    format_config.master_config.addr_len_in_bytes = 1U;
    format_config.common_config.data_len_in_bits = 1;
    format_config.common_config.data_merge = false;
    format_config.common_config.mosi_bidir = false;
    format_config.common_config.lsb = true;
    format_config.common_config.mode = spi_master_mode;
    format_config.common_config.cpol = spi_sclk_low_idle;
    format_config.common_config.cpha = spi_sclk_sampling_odd_clk_edges;
    spi_format_init(JTAG_SPI_BASE, &format_config);

    /* set SPI control config for master */
    spi_master_get_default_control_config(&control_config);
    control_config.master_config.cmd_enable = false;
    control_config.master_config.addr_enable = false;
    control_config.master_config.addr_phase_fmt = spi_address_phase_format_single_io_mode;
    control_config.common_config.trans_mode = spi_trans_write_read_together;
    control_config.common_config.rx_dma_enable = false;
    control_config.common_config.tx_dma_enable = false;
    spi_control_init(JTAG_SPI_BASE, &control_config, 1, 1);
}

#endif  /* (DAP_JTAG != 0) */
