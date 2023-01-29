# Copyright (c) 2022-2023 XiNGRZ
# SPDX-License-Identifier: MIT

# report - ping

zephyr_library_sources(report_ping.c)

# report - fn-state

zephyr_library_sources(report_fn_state.c)
zephyr_library_include_directories(${CMAKE_SOURCE_DIR}/include)
