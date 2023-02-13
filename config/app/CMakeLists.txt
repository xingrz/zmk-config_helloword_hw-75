# Copyright (c) 2022-2023 XiNGRZ
# SPDX-License-Identifier: MIT

zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

zephyr_library_sources_ifdef(CONFIG_SETTINGS storage_init.c)
zephyr_library_sources_ifdef(CONFIG_HW75_HID_MOUSE hid_mouse.c)
zephyr_library_sources_ifdef(CONFIG_HW75_INDICATOR indicator.c)

zephyr_library_sources(behaviors/behavior_sensor_rotate_mouse_wheel.c)

add_subdirectory_ifdef(CONFIG_HW75_USB_COMM usb_comm)
