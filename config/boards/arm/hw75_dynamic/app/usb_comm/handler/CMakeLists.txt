# Copyright (c) 2022-2023 XiNGRZ
# SPDX-License-Identifier: MIT

# handler - version

zephyr_library_sources(handler_version.c)

include(${ZMK_CONFIG}/cmake/version.cmake)
zephyr_library_compile_definitions(
    -DVER_ZEPHYR="${ZEPHYR_VERSION}"
    -DVER_ZMK="${ZMK_VERSION}"
    -DVER_APP="${APP_VERSION}"
)
