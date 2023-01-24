# Copyright (c) 2022-2023 XiNGRZ
# SPDX-License-Identifier: MIT

zephyr_library()

set(NANOPB_DIR ${ZEPHYR_BASE}/../modules/lib/nanopb)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${NANOPB_DIR}/extra)
find_package(Nanopb REQUIRED)
zephyr_library_include_directories(${NANOPB_INCLUDE_DIRS})

nanopb_generate_cpp(PROTO_SRCS PROTO_HDRS
    usb_comm.proto
)
zephyr_library_include_directories(${CMAKE_CURRENT_BINARY_DIR})
zephyr_library_sources(${PROTO_SRCS} ${PROTO_HDRS})

zephyr_library_sources(usb_comm.c)
zephyr_library_sources(usb_comm_bos.c)
zephyr_library_sources(usb_comm_proto.c)

add_subdirectory(handler)
