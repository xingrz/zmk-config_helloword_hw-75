# Copyright (c) 2022-2023 XiNGRZ
# SPDX-License-Identifier: MIT

zephyr_library_include_directories(${CMAKE_SOURCE_DIR}/include)
zephyr_library_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

zephyr_library_sources(ssd1306_delay.c)

zephyr_library_sources(indicator_app.c)

add_subdirectory_ifdef(CONFIG_ZMK_DISPLAY_STATUS_SCREEN_CUSTOM screen)

zephyr_library_include_directories_ifdef(CONFIG_KNOB ${ZMK_CONFIG}/drivers/sensor/knob/include)
zephyr_library_sources_ifdef(CONFIG_KNOB knob_app.c)

add_subdirectory_ifdef(CONFIG_HW75_UART_COMM uart_comm)
add_subdirectory_ifdef(CONFIG_HW75_USB_COMM usb_comm)
