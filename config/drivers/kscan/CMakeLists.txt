# Copyright (c) 2022 The ZMK Contributors
# SPDX-License-Identifier: MIT

zephyr_library_include_directories(${APPLICATION_SOURCE_DIR}/drivers/kscan)

zephyr_library_sources_ifdef(CONFIG_ZMK_KSCAN_GPIO_74HC165 kscan_gpio_74hc165.c)
