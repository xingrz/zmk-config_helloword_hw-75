# Copyright (c) 2022-2023 XiNGRZ
# SPDX-License-Identifier: MIT

if(CONFIG_KNOB)

zephyr_library_named(knob)

zephyr_library_sources(knob.c)
zephyr_library_sources(motor.c)

zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
zephyr_library_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/include)

add_subdirectory(lib)
add_subdirectory(inverter)
add_subdirectory(encoder)
add_subdirectory(profile)

endif()
