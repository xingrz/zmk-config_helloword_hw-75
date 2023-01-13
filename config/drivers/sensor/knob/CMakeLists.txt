# Copyright (c) 2022-2023 XiNGRZ
# SPDX-License-Identifier: MIT

if(CONFIG_KNOB)

zephyr_library_named(helloword__drivers__knob)
zephyr_library_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_subdirectory(lib)
add_subdirectory(encoder)

endif()
