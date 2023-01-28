# Copyright (c) 2022-2023 XiNGRZ
# SPDX-License-Identifier: MIT

zephyr_library_named(hw75_proto)

if(CONFIG_HW75_PROTO)

set(NANOPB_DIR ${ZEPHYR_BASE}/../modules/lib/nanopb)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${NANOPB_DIR}/extra)
find_package(Nanopb REQUIRED)

set(PROTO_DEFS)

if(CONFIG_HW75_PROTO_USB_COMM)
    list(APPEND PROTO_DEFS usb_comm.proto)
endif()

if(CONFIG_HW75_PROTO_UART_COMM)
    list(APPEND PROTO_DEFS uart_comm.proto)
endif()

nanopb_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_DEFS})
zephyr_library_sources(${PROTO_SRCS} ${PROTO_HDRS})

zephyr_include_directories(${NANOPB_INCLUDE_DIRS})
zephyr_include_directories(${CMAKE_CURRENT_BINARY_DIR})

endif() # CONFIG_HW75_PROTO
