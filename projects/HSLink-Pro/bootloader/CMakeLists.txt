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
set(CONFIG_EEPROM_EMULATION 1)

set(HEAP_SIZE 0x1000)
set(STACK_SIZE 0x2000)

set(EXTRA_C_FLAGS "-Werror=return-type -fno-exceptions")

find_package(hpm-sdk REQUIRED HINTS $ENV{HPM_SDK_BASE})

#target_compile_options(hpm_sdk_lib_itf INTERFACE
#        $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti -fno-exceptions>
#)

project(HSLink-Pro-Bootloader)


sdk_app_src(main.c)
sdk_app_src(msc_bootuf2.c)
sdk_app_src(bootuf2/bootuf2.c)
sdk_app_inc(bootuf2)

sdk_inc(.)
sdk_inc(../common)

sdk_compile_definitions(-DPRODUCT_STRING="HSLink Pro")

add_subdirectory(third_party_components)

#generate_ide_projects()