set(THIRD_PARTY_COMPONENTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party_components)

add_subdirectory(${THIRD_PARTY_COMPONENTS_DIR}/neopixel ${CMAKE_CURRENT_BINARY_DIR}/neopixel)

add_subdirectory(${THIRD_PARTY_COMPONENTS_DIR}/tlsf ${CMAKE_CURRENT_BINARY_DIR}/tlsf)
sdk_app_src(port/tlsf/malloc.c)

add_subdirectory(${THIRD_PARTY_COMPONENTS_DIR}/rapidjson ${CMAKE_CURRENT_BINARY_DIR}/rapidjson)

add_subdirectory(${THIRD_PARTY_COMPONENTS_DIR}/multibutton ${CMAKE_CURRENT_BINARY_DIR}/multibutton)

if (CONFIG_USE_ELOG)
    sdk_compile_options(-DCONFIG_USE_ELOG=1)
    sdk_inc(port/easylogger)
    sdk_src(port/easylogger/elog_port.c)
    add_subdirectory(${THIRD_PARTY_COMPONENTS_DIR}/EasyLogger ${CMAKE_CURRENT_BINARY_DIR}/EasyLogger)
endif ()