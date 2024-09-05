cmake_minimum_required(VERSION 3.15)

include(proj.conf)

find_package(bouffalo_sdk REQUIRED HINTS $ENV{BL_SDK_BASE})

target_sources(app PRIVATE
usb2uart.c
${CMAKE_CURRENT_SOURCE_DIR}/../../dap_main.c
${CMAKE_CURRENT_SOURCE_DIR}/../../DAP/Source/DAP_vendor.c
${CMAKE_CURRENT_SOURCE_DIR}/../../DAP/Source/DAP.c
${CMAKE_CURRENT_SOURCE_DIR}/../../DAP/Source/JTAG_DP.c
${CMAKE_CURRENT_SOURCE_DIR}/../../DAP/Source/SW_DP.c
${CMAKE_CURRENT_SOURCE_DIR}/../../CherryRB/chry_ringbuffer.c
${CMAKE_CURRENT_SOURCE_DIR}/../../CherryUSB/core/usbd_core.c
${CMAKE_CURRENT_SOURCE_DIR}/../../CherryUSB/port/bouffalolab/usb_dc_bl.c
${CMAKE_CURRENT_SOURCE_DIR}/../../CherryUSB/class/cdc/usbd_cdc_acm.c
${CMAKE_CURRENT_SOURCE_DIR}/../../CherryUSB/class/hid/usbd_hid.c
)

sdk_add_include_directories(.)
sdk_add_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../CherryRB)
sdk_add_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../..)
sdk_add_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../DAP/Include)

sdk_add_compile_definitions(-DCONFIG_USB_HS)
sdk_add_include_directories(../../CherryUSB/common)
sdk_add_include_directories(../../CherryUSB/core)
sdk_add_include_directories(../../CherryUSB/class/cdc)
sdk_add_include_directories(../../CherryUSB/class/msc)
sdk_add_include_directories(../../CherryUSB/class/hid)

#sdk_add_compile_definitions(-DCONFIG_CHERRYDAP_USE_MSC)

sdk_set_main_file(main.c)

project(cherrydap)
