# Copyright (c) 2022-2023 XiNGRZ
# SPDX-License-Identifier: MIT

add_subdirectory(zfull_9)

zephyr_library_sources(icons_19.c)
zephyr_compile_options(-DLV_LVGL_H_INCLUDE_SIMPLE)
