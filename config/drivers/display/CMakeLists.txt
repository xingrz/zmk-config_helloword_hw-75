# Copyright (c) 2022-2023 XiNGRZ
# SPDX-License-Identifier: MIT

zephyr_library_sources_ifdef(CONFIG_DISPLAY_SW_ROTATE display_sw_rotate.c)
zephyr_library_sources_ifdef(CONFIG_HW75_SSD16XX ssd16xx.c)

zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
