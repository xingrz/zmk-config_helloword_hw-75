# Copyright (c) 2022-2023 XiNGRZ
# SPDX-License-Identifier: MIT

zephyr_library_include_directories(${APPLICATION_SOURCE_DIR}/include)

add_subdirectory_ifdef(CONFIG_HW75_UART_COMM uart_comm)
