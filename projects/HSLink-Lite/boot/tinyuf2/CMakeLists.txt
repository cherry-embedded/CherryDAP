# Copyright (c) 2022 HPMicro
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.13)

if("${HPM_BUILD_TYPE}" STREQUAL "")
    SET(HPM_BUILD_TYPE flash_xip)
endif()

set(CONFIG_TINYUSB 1)
set(CONFIG_USB_DEVICE 1)
set(CONFIG_USB_DEVICE_MSC 1)
set(CONFIG_USB_DEVICE_HID 1)

find_package(hpm-sdk REQUIRED HINTS $ENV{HPM_SDK_BASE})

project(tinyuf2_example)

sdk_compile_definitions(-DUSB_HOST_MCU_CORE=HPM_CORE0)
sdk_compile_options(-O2)

sdk_inc(src)

sdk_app_src(src/main.c)

sdk_app_src(src/ghostfat.c)
sdk_app_src(src/msc.c)
sdk_app_src(src/usb_descriptors.c)
sdk_app_src(src/board_api.c)
generate_ide_projects()
