# Copyright (c) 2022-2023 XiNGRZ
# SPDX-License-Identifier: MIT

zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

zephyr_library_sources_ifdef(CONFIG_SETTINGS storage_init.c)

add_subdirectory_ifdef(CONFIG_HW75_USB_COMM usb_comm)
