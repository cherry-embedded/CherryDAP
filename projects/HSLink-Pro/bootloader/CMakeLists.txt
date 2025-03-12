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

sdk_app_src(src/main.c)
sdk_app_src(src/ws2812.cpp)
sdk_app_src(src/msc_bootuf2.c)
sdk_app_src(bootuf2/bootuf2.c)
sdk_app_inc(bootuf2)

sdk_inc(src)

sdk_inc(../common)

sdk_compile_definitions(-DPRODUCT_STRING="HSLink Pro")

add_subdirectory(../third_party_components/neopixel ${CMAKE_CURRENT_BINARY_DIR}/neopixel)
add_subdirectory(../third_party_components/multibutton ${CMAKE_CURRENT_BINARY_DIR}/multibutton)

#generate_ide_projects()