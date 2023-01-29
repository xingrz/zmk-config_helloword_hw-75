# Copyright (c) 2022-2023 XiNGRZ
# SPDX-License-Identifier: MIT

zephyr_library_sources_ifdef(CONFIG_UART_SLIP uart_slip.c)

zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
