# Copyright (c) 2022-2023 XiNGRZ
# SPDX-License-Identifier: MIT

zephyr_library()

zephyr_library_include_directories(${ZEPHYR_LVGL_MODULE_DIR})
zephyr_library_include_directories(${ZEPHYR_BASE}/lib/gui/lvgl)
zephyr_library_include_directories(${CMAKE_SOURCE_DIR}/include)

zephyr_library_sources(icons_19.c)
zephyr_library_sources(layer_status.c)
zephyr_library_sources(status_screen.c)
