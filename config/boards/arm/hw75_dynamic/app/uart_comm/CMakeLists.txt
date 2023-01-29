# Copyright (c) 2022-2023 XiNGRZ
# SPDX-License-Identifier: MIT

zephyr_library()

zephyr_library_link_libraries(hw75_proto)

zephyr_library_sources(uart_comm.c)

add_subdirectory(handler)
