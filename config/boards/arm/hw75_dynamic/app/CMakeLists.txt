# Copyright (c) 2022-2023 XiNGRZ
# SPDX-License-Identifier: MIT

zephyr_library_sources(ssd1306_delay.c)
zephyr_library_sources(led_indicator_usb.c)

zephyr_library_include_directories(${ZEPHYR_LVGL_MODULE_DIR})
zephyr_library_include_directories(${ZEPHYR_BASE}/lib/gui/lvgl)
zephyr_library_include_directories(${CMAKE_SOURCE_DIR}/include)
zephyr_library_sources_ifdef(CONFIG_ZMK_DISPLAY_STATUS_SCREEN_CUSTOM status_screen.c)
zephyr_library_sources_ifdef(CONFIG_ZMK_DISPLAY_STATUS_SCREEN_CUSTOM widgets/layer_status.c)
zephyr_library_sources_ifdef(CONFIG_ZMK_DISPLAY_STATUS_SCREEN_CUSTOM widgets/icons_19.c)

zephyr_library_include_directories_ifdef(CONFIG_KNOB ${ZMK_CONFIG}/drivers/sensor/knob/include)
zephyr_library_sources_ifdef(CONFIG_KNOB knob_app.c)

add_subdirectory_ifdef(CONFIG_HW75_USB_COMM usb_comm)
