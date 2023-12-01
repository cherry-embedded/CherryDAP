# Copyright (c) 2023 HPMicro
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.13)

set(CONFIG_CHERRYUSB 1)
set(CONFIG_USB_DEVICE 1)
set(CONFIG_USB_DEVICE_CDC 1)

find_package(hpm-sdk REQUIRED HINTS $ENV{HPM_SDK_BASE})

project(hpm5300evklite_jtag_dap)

sdk_compile_definitions(-DCONFIG_USB_HS)
sdk_compile_definitions(-DCONFIG_CHERRYUSB)

sdk_inc(./)
sdk_inc(../..)
sdk_inc(../../CherryRB)
sdk_inc(../../DAP/Include)

sdk_app_src(../../dap_main.c)
sdk_app_src(../../DAP/Source/DAP_vendor.c)
sdk_app_src(../../DAP/Source/DAP.c)
sdk_app_src(../../DAP/Source/SW_DP.c)
sdk_app_src(../../DAP/Source/JTAG_DP.c)
sdk_app_src(../../CherryRB/chry_ringbuffer.c)

sdk_app_src(main.c)
sdk_app_src(usb2uart.c)
sdk_compile_options("-O3")

generate_ses_project()



