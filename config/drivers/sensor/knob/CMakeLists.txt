# Copyright (c) 2022-2023 XiNGRZ
# SPDX-License-Identifier: MIT

if(CONFIG_KNOB)

zephyr_library_named(knob)

zephyr_library_sources(knob.c)
zephyr_library_sources(motor.c)

target_include_directories(knob PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(knob PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib/include)

add_subdirectory(lib)
add_subdirectory(inverter)
add_subdirectory(encoder)

endif()
