set (THIRD_PARTY_COMPONENTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party_components)

sdk_compile_definitions(-DWS2812_USE_SPI=0)
add_subdirectory(${THIRD_PARTY_COMPONENTS_DIR}/WS2812 ${CMAKE_CURRENT_BINARY_DIR}/ws2812)

add_subdirectory(${THIRD_PARTY_COMPONENTS_DIR}/tlsf ${CMAKE_CURRENT_BINARY_DIR}/tlsf)
sdk_app_src(port/tlsf/malloc.c)

add_subdirectory(${THIRD_PARTY_COMPONENTS_DIR}/rapidjson ${CMAKE_CURRENT_BINARY_DIR}/rapidjson)