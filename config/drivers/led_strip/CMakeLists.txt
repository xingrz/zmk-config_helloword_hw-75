# Copyright (c) 2022-2023 XiNGRZ
# SPDX-License-Identifier: MIT

zephyr_library_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

zephyr_library_sources_ifdef(CONFIG_LED_STRIP_REMAP led_strip_remap.c)
