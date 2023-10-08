# Copyright (c) 2022-2023 XiNGRZ
# SPDX-License-Identifier: MIT

zephyr_library()

zephyr_library_include_directories(${ZEPHYR_LVGL_MODULE_DIR})
zephyr_library_include_directories(${ZEPHYR_BASE}/lib/gui/lvgl)
zephyr_library_include_directories(${APPLICATION_SOURCE_DIR}/include)
zephyr_library_include_directories(${BOARD_DIR}/app/include)

include(${ZMK_CONFIG}/cmake/ui_strings.cmake)

generate_ui_strings(icons.csv
    OUTPUT_DEFS ICONS_SRC
    OUTPUT_TEXT ICONS_TXT
    UNICODE_HEX
)

generate_ui_strings(strings.csv
    OUTPUT_DEFS STRINGS_SRC
    OUTPUT_TEXT STRINGS_TXT
)

zephyr_library_sources(${ICONS_SRC} ${STRINGS_SRC})
zephyr_library_include_directories(${CMAKE_CURRENT_BINARY_DIR})

zephyr_library_sources(knob_indicator.c)
zephyr_library_sources(knob_status.c)
zephyr_library_sources(layer_status.c)
zephyr_library_sources(status_screen.c)

add_subdirectory(fonts)
