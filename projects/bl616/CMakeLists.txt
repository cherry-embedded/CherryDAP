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
)

sdk_add_include_directories(.)
sdk_add_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../CherryRB)
sdk_add_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../..)
sdk_add_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../DAP/Include)

set(CONFIG_CHERRYUSB_DEVICE 1)
set(CONFIG_CHERRYUSB_DEVICE_CDC 1)
add_subdirectory(../../CherryUSB CherryUSB)
get_property(target_cherryusb_include_dirs TARGET cherryusb PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
sdk_add_include_directories(${target_cherryusb_include_dirs})
target_link_libraries(cherryusb sdk_intf_lib)
target_link_libraries(app cherryusb)

sdk_add_compile_definitions(-DCONFIG_USB_HS)
sdk_add_compile_definitions(-DCONFIG_CHERRYUSB)
#sdk_add_compile_definitions(-DCONFIG_CHERRYDAP_USE_MSC)

sdk_set_main_file(main.c)

project(cherrydap)
