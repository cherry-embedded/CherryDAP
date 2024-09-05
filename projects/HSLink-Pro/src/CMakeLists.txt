# Copyright (c) 2023 HPMicro
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.13)

option(CONFIG_HSLINK_PRO_EXPANSION "Enable HSLink Pro expansion" OFF)

option(CONFIG_SWDIO_DIR "SWDIO direction" OFF)

set(APP_NAME "HSLink-Pro")

set(CONFIG_USB_DEVICE 1)
set(CONFIG_DMA_MGR 1)

# CONFIG_USE_HPM_BOARD_JTAG_GPIO is for 20p output SWD+JTAG signal of hpm5301evklite
set(CONFIG_USE_HPM_BOARD_JTAG_GPIO 0)

if (NOT DEFINED CONFIG_USE_HPM_BOARD_JTAG_GPIO OR (CONFIG_USE_HPM_BOARD_JTAG_GPIO EQUAL "0"))
    set(CONFIG_USE_SPI_SWD 0)
    set(CONFIG_USE_SPI_JTAG 0)
endif ()

find_package(hpm-sdk REQUIRED HINTS $ENV{HPM_SDK_BASE})

project(HSLink-Pro)

add_subdirectory(../WS2812 ${CMAKE_CURRENT_BINARY_DIR}/ws2812)

# 设置版本号
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/.version)
    file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/.version BUILD_VERSION)
    string(STRIP ${BUILD_VERSION} BUILD_VERSION)
    set(CONFIG_BUILD_VERSION ${BUILD_VERSION})
endif ()

# for sync, use cherryusb from cherryDAP instead of hpm sdk middleware
sdk_compile_definitions(-DCONFIG_USB_HS)
sdk_inc(../../../CherryUSB/common)
sdk_inc(../../../CherryUSB/core)
sdk_inc(../../../CherryUSB/class/cdc)
sdk_inc(../../../CherryUSB/class/msc)
sdk_inc(../../../CherryUSB/class/hid)
sdk_src(../../../CherryUSB/core/usbd_core.c)
sdk_src(../../../CherryUSB/port/hpm/usb_dc_hpm.c)
sdk_src(../../../CherryUSB/class/cdc/usbd_cdc_acm.c)
sdk_src(../../../CherryUSB/class/hid/usbd_hid.c)

# swj_clock default use keil swd frequency boost
sdk_compile_definitions(-DBOOST_KEIL_SWD_FREQ=0)

sdk_compile_definitions(-DPIN_JTAG_TRST=IOC_PAD_PB14)
sdk_compile_definitions(-DPIN_SRST=IOC_PAD_PB15)

sdk_compile_definitions(-DCONFIG_WS2812=1)

if (CONFIG_SWDIO_DIR)
    sdk_compile_definitions(-DSWDIO_DIR=${CONFIG_SWDIO_DIR})
endif ()

if (DEFINED CONFIG_USE_SPI_SWD AND (CONFIG_USE_SPI_SWD EQUAL "1"))
    sdk_compile_definitions(-DUSE_SPI_SWD=1)
    sdk_app_src(SW_DP_SPI.c)
else ()
    sdk_app_src(../../../DAP/Source/SW_DP.c)
endif ()

if (DEFINED CONFIG_USE_SPI_JTAG AND (CONFIG_USE_SPI_JTAG EQUAL "1"))
    sdk_compile_definitions(-DUSE_SPI_JTAG=1)
    sdk_app_src(JTAG_DP_SPI.c)
else ()
    sdk_app_src(../../../DAP/Source/JTAG_DP.c)
endif ()


sdk_inc(.)
sdk_inc(../../..)
sdk_inc(../../../CherryRB)
sdk_inc(../../../DAP/Include)

sdk_inc(../common)

sdk_app_src(../../../dap_main.c)
sdk_app_src(../../../DAP/Source/DAP_vendor.c)
sdk_app_src(../../../DAP/Source/DAP.c)

sdk_app_src(../../../CherryRB/chry_ringbuffer.c)

sdk_app_src(main.c)
sdk_app_src(usb2uart.c)
sdk_app_src(dp_common.c)

if (CONFIG_BUILD_VERSION)
    sdk_compile_definitions(-DDAP_FW_VER="2.1.1")
else ()
    sdk_compile_definitions(-DDAP_FW_VER="Debug")
endif ()

sdk_compile_definitions(-DPRODUCT_STRING="HSLink Pro")
#sdk_compile_definitions(-DCONFIG_USE_HID_CONFIG=1)
sdk_app_src(../common/HSLink_Pro_expansion.c)
sdk_compile_options("-g")

# 执行完之后转换为uf2
add_custom_command(
        TARGET ${APP_ELF_NAME} POST_BUILD
        COMMAND ${python_exec} ${CMAKE_CURRENT_SOURCE_DIR}/../scripts/uf2conv.py -f 0x0A4D5048 -b 0x80020000 -c ${CMAKE_BINARY_DIR}/output/${APP_BIN_NAME} -o ${CMAKE_BINARY_DIR}/output/${APP_NAME}.uf2
        COMMENT "Convert to UF2"
)

#generate_ses_project()