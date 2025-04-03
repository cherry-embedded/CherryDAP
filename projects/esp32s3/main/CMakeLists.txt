# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
file(GLOB_RECURSE SOURCES
    "usb2uart.c"
    "main.c"
    "../../../dap_main.c"
    "../../../DAP/Source/*.c"
    "../../../CherryRB/*.c"
    )

set(include_dirs 
    "."
    "../../../"
    "../../../DAP/Include"
    "../../../CherryRB"
    )
idf_component_register(SRCS ${SOURCES}
                       INCLUDE_DIRS ${include_dirs}
                       REQUIRES esp_http_client nvs_flash driver usb
                       WHOLE_ARCHIVE)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-const-variable -Wno-format -Wno-format-truncation -Wno-incompatible-pointer-types)
