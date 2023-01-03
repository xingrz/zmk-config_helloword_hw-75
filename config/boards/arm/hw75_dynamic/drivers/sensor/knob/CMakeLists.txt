# Copyright (c) 2022-2023 XiNGRZ
# SPDX-License-Identifier: MIT

zephyr_library_named(helloword__drivers__knob)
zephyr_library_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_subdirectory_ifdef(CONFIG_KNOB lib)

add_subdirectory_ifdef(CONFIG_KNOB encoder)
