/*
 * Copyright (c) 2024 RCSN
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "DAP_config.h"
#include "DAP.h"
#include "hpm_spi_drv.h"

void set_swj_clock_frequency(uint32_t clock)
{
    uint8_t div, sclk_div;
    uint32_t div_remainder;
    uint32_t div_integer;
    uint32_t clk_src_freq_in_hz, sclk_freq_in_hz;
    sclk_freq_in_hz = clock;
    SPI_Type *spi_base = NULL;
    clock_name_t clock_name;
    //    clk_src_freq_in_hz = clock_get_frequency(JTAG_SPI_BASE_CLOCK_NAME);
    clk_src_t src_clock = clk_src_pll1_clk0; /* 800M */
    if (DAP_Data.debug_port == DAP_PORT_SWD) {
        spi_base = SWD_SPI_BASE;
        clock_name = SWD_SPI_BASE_CLOCK_NAME;
    } else {
        spi_base = JTAG_SPI_BASE;
        clock_name = JTAG_SPI_BASE_CLOCK_NAME;
    }
    if (clock >= 40000000) { /* >= 40M*/
            sclk_div = 0xFF;
            div = clock_get_frequency(clk_pll1clk0) / clock;
        } else if ((clock >= 10000000)) { /* 10M <= clock < 40M */
            div = 10;
            clk_src_freq_in_hz = clock_get_frequency(clk_pll1clk0) / div;
            div_remainder = (clk_src_freq_in_hz % sclk_freq_in_hz);
            div_integer = (clk_src_freq_in_hz / sclk_freq_in_hz);
            if ((div_remainder != 0) || ((div_integer % 2) != 0)) {
                sclk_div = 0;
                clk_src_freq_in_hz = sclk_freq_in_hz * 2;
                div = clock_get_frequency(clk_pll1clk0) / clk_src_freq_in_hz;
            } else {
                sclk_div = (div_integer / 2) - 1;
                div = 10;
            }
        } else { /* < 10M */
            sclk_div = 3;
            clk_src_freq_in_hz = sclk_freq_in_hz * 8;
            div = clock_get_frequency(clk_pll1clk0) / clk_src_freq_in_hz;
        }

    spi_master_set_sclk_div(spi_base, sclk_div);
    clock_set_source_divider(clock_name, src_clock, div);
}
