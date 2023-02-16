# Copyright (c) 2022-2023 XiNGRZ
# SPDX-License-Identifier: MIT

zephyr_library()

zephyr_library_link_libraries(hw75_proto)

zephyr_library_sources(usb_comm_hid.c)
zephyr_library_sources(usb_comm_proto.c)

zephyr_linker_sources(DATA_SECTIONS usb_comm_handler.ld)

add_subdirectory(handler)
