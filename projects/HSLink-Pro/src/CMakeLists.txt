# Copyright (c) 2023 HPMicro
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.13)

set(APP_NAME "HSLink-Pro")
set(BOARD_SEARCH_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../common)

set(CONFIG_USB_DEVICE 1)
set(CONFIG_DMA_MGR 1)
set(CONFIG_HPM_SPI 1)

set(CONFIG_USE_ELOG 1)

set(CUSTOM_GCC_LINKER_FILE
        ${CMAKE_CURRENT_SOURCE_DIR}/flash_uf2.ld
)

#set(CONFIG_EEPROM_EMULATION 1)
set(STACK_SIZE 0x5000)

#set(HPM_SDK_LD_NO_NANO_SPECS 1)

find_package(hpm-sdk REQUIRED HINTS $ENV{HPM_SDK_BASE})

project(HSLink-Pro)

# 设置版本号
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/.version)
    file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/.version BUILD_VERSION)
    string(STRIP ${BUILD_VERSION} BUILD_VERSION)
    set(CONFIG_BUILD_VERSION ${BUILD_VERSION})
endif ()

if (CONFIG_BUILD_VERSION)
    sdk_compile_definitions(-DCONFIG_BUILD_VERSION="${CONFIG_BUILD_VERSION}")
    string(REGEX REPLACE "([0-9]+)\\..*" "\\1" VERSION_MAJOR ${CONFIG_BUILD_VERSION})
    string(REGEX REPLACE "[0-9]+\\.([0-9]+)\\..*" "\\1" VERSION_MINOR ${CONFIG_BUILD_VERSION})
    string(REGEX REPLACE "[0-9]+\\.[0-9]+\\.([0-9]+)" "\\1" VERSION_PATCH ${CONFIG_BUILD_VERSION})
    sdk_compile_definitions(-DCONFIG_BUILD_VERSION_MAJOR=${VERSION_MAJOR})
    sdk_compile_definitions(-DCONFIG_BUILD_VERSION_MINOR=${VERSION_MINOR})
    sdk_compile_definitions(-DCONFIG_BUILD_VERSION_PATCH=${VERSION_PATCH})
else ()
    sdk_compile_definitions(-DCONFIG_BUILD_VERSION="Debug")
    sdk_compile_definitions(-DCONFIG_BUILD_VERSION_MAJOR=0xFF)
    sdk_compile_definitions(-DCONFIG_BUILD_VERSION_MINOR=0xFF)
    sdk_compile_definitions(-DCONFIG_BUILD_VERSION_PATCH=0xFF)
endif ()

# for sync, use cherryusb from cherryDAP instead of hpm sdk middleware
sdk_inc(../../../CherryUSB/common)
sdk_inc(../../../CherryUSB/core)
sdk_inc(../../../CherryUSB/class/cdc)
sdk_inc(../../../CherryUSB/class/msc)
sdk_inc(../../../CherryUSB/class/hid)
sdk_src(../../../CherryUSB/core/usbd_core.c)
sdk_src(../../../CherryUSB/port/hpm/usb_dc_hpm.c)
sdk_src(../../../CherryUSB/class/cdc/usbd_cdc_acm.c)
sdk_src(../../../CherryUSB/class/hid/usbd_hid.c)

sdk_compile_definitions(-DPIN_JTAG_TRST=IOC_PAD_PB14)
sdk_compile_definitions(-DPIN_SRST=IOC_PAD_PB15)
sdk_compile_definitions(-DSWDIO_DIR=IOC_PAD_PA30)

sdk_app_inc(SW_DP)
sdk_app_src(SW_DP/SW_DP.c)
sdk_app_src(SW_DP/SW_DP_SPI.c)
sdk_app_src(SW_DP/SW_DP_IO.c)

sdk_app_inc(JTAG_DP)
sdk_app_src(JTAG_DP/JTAG_DP.c)
sdk_app_src(JTAG_DP/JTAG_DP_SPI.c)
sdk_app_src(JTAG_DP/JTAG_DP_IO.c)

sdk_app_src(swd_host/swd_host.c)
sdk_app_inc(swd_host)
sdk_app_src(USB2UART/usb2uart.c)
sdk_app_inc(USB2UART)

sdk_inc(.)
sdk_inc(../../..)
sdk_inc(../../../CherryRB)
sdk_inc(../../../DAP/Include)

sdk_app_src(../../../dap_main.c)
sdk_app_src(../../../DAP/Source/DAP_vendor.c)
sdk_app_src(../../../DAP/Source/DAP.c)

sdk_app_src(../../../CherryRB/chry_ringbuffer.c)

sdk_inc(../common)

sdk_app_src(main.cpp)
sdk_app_src(dp_common.c)
sdk_app_src(reset_way.cpp)
sdk_app_src(setting.cpp)
sdk_app_src(led_extern.cpp)

sdk_compile_definitions(-DPRODUCT_STRING="HSLink Pro")
sdk_compile_definitions(-DCONFIG_CHERRYDAP_USE_CUSTOM_HID=1)
sdk_app_src(HSLink_Pro_expansion.cpp)

file(GLOB_RECURSE HID_COMM_SRC
        "HID_COMM/*.c"
        "HID_COMM/*.cpp"
)
sdk_app_src(${HID_COMM_SRC})
sdk_app_inc(HID_COMM)

sdk_compile_options(-O2)

sdk_ld_options("-lm")

#sdk_compile_definitions("WS2812_TEST")

add_subdirectory(third_party_components)

# 执行完之后转换为uf2
add_custom_command(
        TARGET ${APP_ELF_NAME} POST_BUILD
        COMMAND ${python_exec} ${CMAKE_CURRENT_SOURCE_DIR}/../scripts/uf2conv.py -f 0x0A4D5048 -b 0x80020000 -c ${CMAKE_BINARY_DIR}/output/${APP_BIN_NAME} -o ${CMAKE_BINARY_DIR}/output/${APP_NAME}.uf2
        COMMENT "Convert to UF2"
)

#generate_ide_projects()
