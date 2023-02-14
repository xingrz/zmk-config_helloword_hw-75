# Copyright (c) 2022-2023 XiNGRZ
# SPDX-License-Identifier: MIT

add_subdirectory(mono_19)
add_subdirectory(zfull_9)

zephyr_compile_options(-DLV_LVGL_H_INCLUDE_SIMPLE)
