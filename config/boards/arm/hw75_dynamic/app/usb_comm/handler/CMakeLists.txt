# Copyright (c) 2022-2023 XiNGRZ
# SPDX-License-Identifier: MIT

# handler - version

zephyr_library_sources(handler_version.c)

include(${ZMK_CONFIG}/cmake/version.cmake)
zephyr_library_compile_definitions(
    -DVER_ZEPHYR="${ZEPHYR_VERSION}"
    -DVER_ZMK="${ZMK_VERSION}"
    -DVER_APP="${APP_VERSION}"
)

# handler - knob

zephyr_library_sources(handler_knob.c)
zephyr_library_include_directories(${ZMK_CONFIG}/drivers/sensor/knob/include)
zephyr_library_include_directories(${ZMK_CONFIG}/boards/arm/hw75_dynamic/app)

# handler - rgb

zephyr_library_sources(handler_rgb.c)
zephyr_library_include_directories(${CMAKE_SOURCE_DIR}/include)

# handler - eink
zephyr_library_sources(handler_eink.c)
zephyr_library_include_directories(${ZMK_CONFIG}/drivers/display/include)
