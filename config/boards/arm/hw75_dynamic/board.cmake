# Copyright (c) 2022 The ZMK Contributors
# SPDX-License-Identifier: MIT

board_runner_args(pyocd "--target=stm32f405rgtx")
board_runner_args(dfu-util "--pid=0483:df11" "--alt=0" "--dfuse")

set_ifndef(BOARD_FLASH_RUNNER pyocd)

include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/dfu-util.board.cmake)
