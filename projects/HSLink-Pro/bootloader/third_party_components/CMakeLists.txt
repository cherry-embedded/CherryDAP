set(THIRD_PARTY_COMPONENTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party_components)

add_subdirectory(${THIRD_PARTY_COMPONENTS_DIR}/neopixel ${CMAKE_CURRENT_BINARY_DIR}/neopixel)
sdk_app_src(port/neopixel/ws2812.cpp)
sdk_app_inc(port/neopixel)

add_subdirectory(${THIRD_PARTY_COMPONENTS_DIR}/multibutton ${CMAKE_CURRENT_BINARY_DIR}/multibutton)

sdk_inc(../../common/easylogger)
sdk_src(../../common/easylogger/elog_port.c)
add_subdirectory(${THIRD_PARTY_COMPONENTS_DIR}/EasyLogger ${CMAKE_CURRENT_BINARY_DIR}/EasyLogger)

sdk_compile_definitions(-DFAL_PART_HAS_TABLE_CFG)
sdk_inc(../../common/fal)
sdk_src(../../common/fal/fal_hpm_internal_port.c)
sdk_src(../../common/fal/fal_parts.c)
add_subdirectory(${THIRD_PARTY_COMPONENTS_DIR}/fal ${CMAKE_CURRENT_BINARY_DIR}/fal)

sdk_inc(../../common/flashdb)
add_subdirectory(${THIRD_PARTY_COMPONENTS_DIR}/flashdb ${CMAKE_CURRENT_BINARY_DIR}/flashdb)