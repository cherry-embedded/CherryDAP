cmake_minimum_required(VERSION 3.13)

set(CONFIG_CHERRYUSB 1)
set(CONFIG_USB_DEVICE 1)
set(CONFIG_USB_DEVICE_MSC 1)
set(CUSTOM_GCC_LINKER_FILE
    ${CMAKE_CURRENT_SOURCE_DIR}/flash_xip.ld
)

find_package(hpm-sdk REQUIRED HINTS $ENV{HPM_SDK_BASE})

project(CherryUSB-UF2)

add_subdirectory(WS2812)

sdk_compile_definitions(
#        -DBOARD_SHOW_CLOCK=0
        -DBOARD_SHOW_BANNER=0
)

sdk_app_src(src/main.c)
sdk_app_src(src/msc_bootuf2.c)
sdk_app_src(bootuf2/bootuf2.c)
sdk_app_inc(bootuf2)

sdk_inc(src)

#generate_ide_projects()