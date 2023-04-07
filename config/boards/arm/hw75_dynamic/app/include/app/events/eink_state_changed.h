/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/kernel.h>
#include <zmk/event_manager.h>

struct app_eink_state_changed {
	bool busy;
};

ZMK_EVENT_DECLARE(app_eink_state_changed);
