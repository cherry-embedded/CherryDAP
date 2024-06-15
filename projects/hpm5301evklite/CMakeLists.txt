# Copyright (c) 2023 HPMicro
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.13)

set(CONFIG_USB_DEVICE 1)
set(CONFIG_DMA_MGR 1)
# set(CONFIG_USE_SPI_SWD 1)
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


if(CONFIG_USE_SPI_SWD)
sdk_compile_definitions(-DUSE_SPI_SWD=1)
sdk_app_src(SW_DP_SPI.c)
else()
sdk_app_src(../../DAP/Source/SW_DP.c)
endif()

sdk_inc(./)
sdk_inc(../..)
sdk_inc(../../CherryRB)
sdk_inc(../../DAP/Include)

sdk_app_src(../../dap_main.c)
sdk_app_src(../../DAP/Source/DAP_vendor.c)
sdk_app_src(../../DAP/Source/DAP.c)

sdk_app_src(../../DAP/Source/JTAG_DP.c)
sdk_app_src(../../CherryRB/chry_ringbuffer.c)

sdk_app_src(main.c)
sdk_app_src(usb2uart.c)
sdk_compile_options("-O3")

generate_ses_project()



