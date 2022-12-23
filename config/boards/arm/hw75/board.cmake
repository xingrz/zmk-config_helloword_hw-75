# Copyright (c) 2022 The ZMK Contributors
# SPDX-License-Identifier: MIT

board_runner_args(pyocd "--target=stm32f103c8")
board_runner_args(dfu-util "--pid=dead:ca5d" "--alt=0" "--dfuse" "--dfuse-modifiers=will-reset")

set_ifndef(BOARD_FLASH_RUNNER dfu-util)

include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/dfu-util.board.cmake)
