set(THIRD_PARTY_COMPONENTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party_components)

add_subdirectory(${THIRD_PARTY_COMPONENTS_DIR}/neopixel ${CMAKE_CURRENT_BINARY_DIR}/neopixel)
sdk_app_src(port/neopixel/ws2812.cpp)
sdk_app_inc(port/neopixel)

add_subdirectory(${THIRD_PARTY_COMPONENTS_DIR}/multibutton ${CMAKE_CURRENT_BINARY_DIR}/multibutton)