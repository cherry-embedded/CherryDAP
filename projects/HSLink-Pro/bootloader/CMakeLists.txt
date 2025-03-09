cmake_minimum_required(VERSION 3.13)

set(APP_NAME HSLink-Pro-Bootloader)
set(BOARD_SEARCH_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../common)

set(CONFIG_DMA_MGR 1)

set(CONFIG_CHERRYUSB 1)
set(CONFIG_USB_DEVICE 1)
set(CONFIG_USB_DEVICE_MSC 1)
set(CUSTOM_GCC_LINKER_FILE
    ${CMAKE_CURRENT_SOURCE_DIR}/flash_xip.ld
)

find_package(hpm-sdk REQUIRED HINTS $ENV{HPM_SDK_BASE})

project(HSLink-Pro-Bootloader)

#sdk_compile_definitions(-DCONFIG_WS2812=1)
#sdk_compile_definitions(-DWS2812_USE_SPI=0)
#add_subdirectory(../third_party_components/WS2812 ${CMAKE_CURRENT_BINARY_DIR}/ws2812)

sdk_app_src(src/main.c)
sdk_app_src(src/msc_bootuf2.c)
sdk_app_src(bootuf2/bootuf2.c)
sdk_app_inc(bootuf2)

sdk_inc(src)

sdk_inc(../common)

sdk_compile_definitions(-DPRODUCT_STRING="HSLink Pro")

#generate_ide_projects()