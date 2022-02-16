idf_component_register(
  SRCS
    "canokey-esp32.c"
    "usbd_conf.c"
    "dcd_esp32sx.c"
  PRIV_INCLUDE_DIRS "."
  PRIV_REQUIRES "usb")

# canokey-core and esp-idf both have mbedtls
set(MBEDTLS_TARGET_PREFIX "canokey-")
add_definitions(-DMBEDTLS_CONFIG_FILE="${CMAKE_CURRENT_SOURCE_DIR}/mbedtls-config.h")
add_definitions(-DDEVICE_CONFIG_FILE="${CMAKE_CURRENT_SOURCE_DIR}/device-config.h")
add_subdirectory(canokey-core)

target_link_libraries(${COMPONENT_LIB} PRIVATE canokey-core c m nosys)
target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-error=discarded-qualifiers")
