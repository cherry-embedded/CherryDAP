# Copyright (c) 2023 HPMicro
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.13)

set(CONFIG_USB_DEVICE 1)
set(CONFIG_DMA_MGR 1)

# CONFIG_USE_HPM_BOARD_JTAG_GPIO is for 20p output SWD+JTAG signal of hpm5301evklite
set(CONFIG_USE_HPM_BOARD_JTAG_GPIO 0)

if(NOT DEFINED CONFIG_USE_HPM_BOARD_JTAG_GPIO OR (CONFIG_USE_HPM_BOARD_JTAG_GPIO EQUAL "0"))
    set(CONFIG_USE_SPI_SWD 1)
    set(CONFIG_USE_SPI_JTAG 1)
endif()

find_package(hpm-sdk REQUIRED HINTS $ENV{HPM_SDK_BASE})

project(hpm5300evklite_dap)

# for sync, use cherryusb from cherryDAP instead of hpm sdk middleware
sdk_compile_definitions(-DCONFIG_USB_HS)
sdk_inc(../../CherryUSB/common)
sdk_inc(../../CherryUSB/core)
sdk_inc(../../CherryUSB/class/cdc)
sdk_inc(../../CherryUSB/class/msc)
sdk_src(../../CherryUSB/core/usbd_core.c)
sdk_src(../../CherryUSB/port/hpm/usb_dc_hpm.c)
sdk_src(../../CherryUSB/class/cdc/usbd_cdc.c)

# swj_clock default use keil swd frequency boost
sdk_compile_definitions(-DBOOST_KEIL_SWD_FREQ=1)

if(NOT DEFINED CONFIG_USE_HPM_BOARD_JTAG_GPIO AND (CONFIG_USE_HPM_BOARD_JTAG_GPIO EQUAL "1"))
sdk_compile_definitions(-DUSE_HPM_BOARD_JTAG_GPIO=1)
sdk_compile_definitions(-DPIN_LED_CONNECTED=IOC_PAD_PA10)
sdk_compile_definitions(-DPIN_LED_RUNNING=IOC_PAD_PA10)
sdk_compile_definitions(-DPIN_JTAG_TRST=IOC_PAD_PA08)
sdk_compile_definitions(-DPIN_SRST=IOC_PAD_PA08)
sdk_compile_definitions(-DLED_ACTIVE_LEVEL=1)
else()
sdk_compile_definitions(-DPIN_LED_CONNECTED=IOC_PAD_PA10)
sdk_compile_definitions(-DPIN_LED_RUNNING=IOC_PAD_PA10)
sdk_compile_definitions(-DPIN_JTAG_TRST=IOC_PAD_PB08)
sdk_compile_definitions(-DPIN_SRST=IOC_PAD_PB09)
sdk_compile_definitions(-DLED_ACTIVE_LEVEL=1)
endif()

if(DEFINED CONFIG_USE_SPI_SWD AND (CONFIG_USE_SPI_SWD EQUAL "1"))
sdk_compile_definitions(-DUSE_SPI_SWD=1)
sdk_app_src(SW_DP_SPI.c)
else()
sdk_app_src(../../DAP/Source/SW_DP.c)
endif()

if(DEFINED CONFIG_USE_SPI_JTAG AND (CONFIG_USE_SPI_JTAG EQUAL "1"))
sdk_compile_definitions(-DUSE_SPI_JTAG=1)
sdk_app_src(JTAG_DP_SPI.c)
else()
sdk_app_src(../../DAP/Source/JTAG_DP.c)
endif()

sdk_inc(./)
sdk_inc(../..)
sdk_inc(../../CherryRB)
sdk_inc(../../DAP/Include)

sdk_app_src(../../dap_main.c)
sdk_app_src(../../DAP/Source/DAP_vendor.c)
sdk_app_src(../../DAP/Source/DAP.c)

sdk_app_src(../../CherryRB/chry_ringbuffer.c)

sdk_app_src(main.c)
sdk_app_src(usb2uart.c)
sdk_app_src(dp_common.c)
sdk_compile_options("-O3")

generate_ses_project()



