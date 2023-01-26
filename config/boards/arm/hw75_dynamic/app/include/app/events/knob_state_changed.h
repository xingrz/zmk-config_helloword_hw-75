/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr.h>
#include <zmk/event_manager.h>

#include <knob/drivers/knob.h>

enum knob_calibration_state {
	KNOB_CALIBRATING,
	KNOB_CALIBRATE_OK,
	KNOB_CALIBRATE_FAILED,
};

struct app_knob_state_changed {
	bool enable;
	bool demo;
	enum knob_calibration_state calibration;
};

ZMK_EVENT_DECLARE(app_knob_state_changed);
