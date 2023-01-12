# Copyright (c) 2022 The ZMK Contributors
# SPDX-License-Identifier: MIT

add_subdirectory_ifdef(CONFIG_BOARD_HW75 app)
add_subdirectory_ifdef(CONFIG_BOARD_HW75 drivers)
