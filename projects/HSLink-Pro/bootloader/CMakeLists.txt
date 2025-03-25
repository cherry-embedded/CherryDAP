cmake_minimum_required(VERSION 3.13)

set(APP_NAME HSLink-Pro-Bootloader)
set(BOARD_SEARCH_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../common)

set(CONFIG_DMA_MGR 1)
set(CONFIG_HPM_SPI 1)

set(CONFIG_CHERRYUSB 1)
set(CONFIG_USB_DEVICE 1)
set(CONFIG_USB_DEVICE_MSC 1)
set(CUSTOM_GCC_LINKER_FILE
        ${CMAKE_CURRENT_SOURCE_DIR}/flash_xip.ld
)

set(HEAP_SIZE 0x8000)
set(STACK_SIZE 0x2000)

set(EXTRA_C_FLAGS "-Werror=return-type -fno-exceptions")

find_package(hpm-sdk REQUIRED HINTS $ENV{HPM_SDK_BASE})

project(HSLink-Pro-Bootloader)

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../src/.version)
    file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/../src/.version BUILD_VERSION)
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

sdk_app_src(main.c)
sdk_app_src(msc_bootuf2.c)
sdk_app_src(bootuf2/bootuf2.c)
sdk_app_inc(bootuf2)

sdk_inc(.)
sdk_inc(../common)

sdk_compile_definitions(-DPRODUCT_STRING="HSLink Pro")

sdk_compile_options(-O2)

add_subdirectory(third_party_components)

#generate_ide_projects()