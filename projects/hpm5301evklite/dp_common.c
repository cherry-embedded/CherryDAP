/*
 * Copyright (c) 2024 RCSN
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "DAP_config.h"
#include "DAP.h"
#include "hpm_spi_drv.h"

#define SPI_MAX_SRC_CLOCK 80000000
#define SPI_MID_SRC_CLOCK 60000000
#define SPI_MIN_SRC_CLOCK 50000000
void set_swj_clock_frequency(uint32_t clock)
{
    uint32_t div, sclk_div;
    uint32_t sclk_freq_in_hz;
    sclk_freq_in_hz = clock;
    SPI_Type *spi_base = NULL;
    clock_name_t clock_name;
    clk_src_t src_clock = clk_src_pll1_clk0; /* 800M */
    if (BOOST_KEIL_SWD_FREQ == 1) {
        sclk_freq_in_hz *= 10;
    }
    PORT_Mode_t mode;
    if (DAP_Data.debug_port == DAP_PORT_SWD) {
        if (sclk_freq_in_hz < 100000) {
            mode = PORT_MODE_GPIO;
        } else {
            mode = PORT_MODE_SPI;
        }

        // 判断是否需要切换模式
        if (SWD_Port_Mode != mode) {
            SWD_Port_Mode = mode;
            PORT_SWD_SETUP();
        }

        spi_base = SWD_SPI_BASE;
        clock_name = SWD_SPI_BASE_CLOCK_NAME;
    } else {
        if (sclk_freq_in_hz < 100000) {
            mode = PORT_MODE_GPIO;
        } else {
            mode = PORT_MODE_SPI;
        }

        // 判断是否需要切换模式
        if (JTAG_Port_Mode != mode) {
            JTAG_Port_Mode = mode;
            PORT_JTAG_SETUP();
        }

        spi_base = JTAG_SPI_BASE;
        clock_name = JTAG_SPI_BASE_CLOCK_NAME;
    }

    if (mode == PORT_MODE_GPIO) {
        Set_Clock_Delay(sclk_freq_in_hz);
        return;
    }

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
    spi_master_set_sclk_div(spi_base, sclk_div);
    clock_set_source_divider(clock_name, src_clock, div);
}
