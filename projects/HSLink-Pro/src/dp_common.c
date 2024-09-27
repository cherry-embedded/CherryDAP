/*
 * Copyright (c) 2024 RCSN
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "DAP_config.h"
#include "DAP.h"
#include "hpm_spi_drv.h"
#include "hpm_clock_drv.h"
#include "swd_host.h"
#include <stdlib.h>

#define SPI_MAX_SRC_CLOCK  (80000000U)
#define SPI_MID_SRC_CLOCK  (60000000U)
#define SPI_MIN_SRC_CLOCK  (50000000U)
#define SPI_MIN_SCLK_CLOCK (20000000U)

void set_swj_clock_frequency(uint32_t clock)
{
    int freq_list[clock_source_general_source_end] = {0};
    uint32_t div, sclk_div;
    uint32_t sclk_freq_in_hz;
    sclk_freq_in_hz = clock;
    SPI_Type *spi_base = NULL;
    clock_name_t clock_name;
    clock_source_t clock_source;
    uint32_t pll_clk = 0;
    int min_diff_freq;
    int current_diff_freq;
    int best_freq;
    uint8_t i;
    int _freq = sclk_freq_in_hz;
    clk_src_t src_clock = clk_src_pll1_clk0; /* 800M */
    if (BOOST_KEIL_SWD_FREQ == 1) {
        sclk_freq_in_hz *= 10;
    }
    if (DAP_Data.debug_port == DAP_PORT_SWD) {
        spi_base = SWD_SPI_BASE;
        clock_name = SWD_SPI_BASE_CLOCK_NAME;
    } else {
        spi_base = JTAG_SPI_BASE;
        clock_name = JTAG_SPI_BASE_CLOCK_NAME;
    }
    if (sclk_freq_in_hz <= SPI_MIN_SCLK_CLOCK) {
        sclk_div = ((SPI_MAX_SRC_CLOCK / sclk_freq_in_hz) / 2) - 1; /* SCLK = SPI_SRC_CLOK / ((SCLK_DIV + 1) * 2)*/
        if (sclk_div <= 0xFE) {
            div = 10;
        } else {
            div = 10;
            src_clock = clk_src_pll0_clk1;                              /* 600M */
            sclk_div = ((SPI_MID_SRC_CLOCK / sclk_freq_in_hz) / 2) - 1; /* SCLK = SPI_SRC_CLOK / ((SCLK_DIV + 1) * 2)*/
            if (sclk_div >= 0xFE) {
                div = 10;
                src_clock = clk_src_pll1_clk2;  /* 500M */
                sclk_div = ((SPI_MIN_SRC_CLOCK / sclk_freq_in_hz) / 2) - 1;
                if (sclk_div >= 0xFE) {
                    sclk_div = 0xFE; /* The minimum sclk clock allowed is 98KHz */
                }
            }
        }
    } else {
        sclk_div = 0xFF;
        for (clock_source = (clock_source_t) 0; clock_source < clock_source_general_source_end; clock_source++) {
            pll_clk = get_frequency_for_source(clock_source);
            div = pll_clk / sclk_freq_in_hz;
            /* The division factor ranges from 1 to 256 as any integer */
            if ((div > 0) && (div <= 0x100)) {
                freq_list[clock_source] = pll_clk / div;
            }
        }
        /* Find the best sclk frequency */
        min_diff_freq = abs(freq_list[0] - _freq);
        best_freq = freq_list[0];
        for (i = 1; i < clock_source_general_source_end; i++) {
            current_diff_freq = abs(freq_list[i] - _freq);
            if (current_diff_freq < min_diff_freq) {
                min_diff_freq = current_diff_freq;
                best_freq = freq_list[i];
            }
        }
        /* Find the best spi clock frequency */
        for (i = 0; i < clock_source_general_source_end; i++) {
            if (best_freq == freq_list[i]) {
                pll_clk = get_frequency_for_source((clock_source_t) i);
                src_clock = MAKE_CLK_SRC(CLK_SRC_GROUP_COMMON, i);
                div = pll_clk / best_freq;
                break;
            }
        }
    }
    spi_master_set_sclk_div(spi_base, sclk_div);
    clock_set_source_divider(clock_name, src_clock, div);
}

// Use the CMSIS-Core definition if available.
#if !defined(SCB_AIRCR_PRIGROUP_Pos)
#define SCB_AIRCR_PRIGROUP_Pos              8U                                            /*!< SCB AIRCR: PRIGROUP Position */
#define SCB_AIRCR_PRIGROUP_Msk             (7UL << SCB_AIRCR_PRIGROUP_Pos)                /*!< SCB AIRCR: PRIGROUP Mask */
#endif

uint8_t software_reset(void)
{
    if (DAP_Data.debug_port != DAP_PORT_SWD) {
        return 1;
    }
    uint8_t ret = 0;
    uint32_t val;
    ret |= swd_read_word(NVIC_AIRCR, &val);
    ret |= swd_write_word(NVIC_AIRCR, VECTKEY | (val & SCB_AIRCR_PRIGROUP_Msk) | SYSRESETREQ);
    return ret;
}
