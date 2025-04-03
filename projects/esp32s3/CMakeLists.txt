# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

add_definitions(-DCONFIG_CHERRYUSB_DEVICE_DCD="dwc2_esp")
add_definitions(-DCONFIG_USBDEV_ADVANCE_DESC=1)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(CherryDAP)
