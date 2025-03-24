set(THIRD_PARTY_COMPONENTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party_components)

add_subdirectory(${THIRD_PARTY_COMPONENTS_DIR}/neopixel ${CMAKE_CURRENT_BINARY_DIR}/neopixel)

add_subdirectory(${THIRD_PARTY_COMPONENTS_DIR}/tlsf ${CMAKE_CURRENT_BINARY_DIR}/tlsf)
sdk_app_src(port/tlsf/malloc.c)

add_subdirectory(${THIRD_PARTY_COMPONENTS_DIR}/rapidjson ${CMAKE_CURRENT_BINARY_DIR}/rapidjson)

add_subdirectory(${THIRD_PARTY_COMPONENTS_DIR}/multibutton ${CMAKE_CURRENT_BINARY_DIR}/multibutton)

sdk_inc(../../common/easylogger)
sdk_src(../../common/easylogger/elog_port.c)
add_subdirectory(${THIRD_PARTY_COMPONENTS_DIR}/EasyLogger ${CMAKE_CURRENT_BINARY_DIR}/EasyLogger)

sdk_inc(../../common/fal)
sdk_src(../../common/fal/fal_hpm_internal_port.c)
sdk_src(../../common/fal/fal_parts.c)
add_subdirectory(${THIRD_PARTY_COMPONENTS_DIR}/fal ${CMAKE_CURRENT_BINARY_DIR}/fal)