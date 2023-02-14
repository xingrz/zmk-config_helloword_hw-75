# Copyright (c) 2022-2023 XiNGRZ
# SPDX-License-Identifier: MIT

zephyr_library()

set(FONT_SRC icons.ttf)

set(FONT_NAME mono)
set(FONT_SIZE 19)

include(${ZMK_CONFIG}/cmake/font_subset.cmake)
font_subset(FONT_SUBSET_TTF ${FONT_SRC} ${ICONS_TXT})

include(${ZMK_CONFIG}/cmake/lv_font_conv.cmake)
lv_font_conv(FONT_C_SRCS ${FONT_SUBSET_TTF}
    NAME ${FONT_NAME}
    SIZE ${FONT_SIZE}
    BPP 1
    NO_PREFILTER
    NO_KERNING
)

zephyr_library_sources(${FONT_C_SRCS})
zephyr_library_include_directories(${ZEPHYR_LVGL_MODULE_DIR})
zephyr_library_include_directories(${ZEPHYR_BASE}/lib/gui/lvgl)
