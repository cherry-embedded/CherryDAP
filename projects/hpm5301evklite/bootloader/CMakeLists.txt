cmake_minimum_required(VERSION 3.13)

set(CONFIG_CHERRYUSB 1)
set(CONFIG_USB_DEVICE 1)
set(CONFIG_USB_DEVICE_MSC 1)

find_package(hpm-sdk REQUIRED HINTS $ENV{HPM_SDK_BASE})

project(CherryUSB-UF2)

sdk_app_src(src/main.c)
sdk_app_src(src/msc_bootuf2.c)
sdk_app_src(bootuf2/bootuf2.c)
sdk_app_inc(bootuf2)

sdk_inc(src)

#generate_ide_projects()