/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#include "handler.h"
#include "usb_comm.pb.h"

#include <device.h>
#include <knob/drivers/knob.h>
#include <knob/drivers/motor.h>

static const struct device *knob;
static const struct device *motor;

static struct motor_state state = {};

bool handle_motor_get_state(MotorState *res)
{
	if (!motor) {
		return false;
	}

	motor_inspect(motor, &state);

	res->timestamp = state.timestamp;
	res->control_mode = (MotorState_ControlMode)state.control_mode;
	res->current_angle = state.current_angle;
	res->current_velocity = state.current_velocity;
	res->target_angle = state.target_angle;
	res->target_velocity = state.target_velocity;
	res->target_voltage = state.target_voltage;

	return true;
}

bool handle_knob_get_config(KnobConfig *res)
{
	if (!knob) {
		return false;
	}

	res->demo = !knob_get_encoder_report(knob);
	res->mode = (KnobConfig_Mode)knob_get_mode(knob);

	if (res->mode == KnobConfig_Mode_DAMPED) {
		knob_get_position_limit(knob, &res->damped_min, &res->damped_max);
		res->has_damped_min = true;
		res->has_damped_max = true;
	} else {
		res->has_damped_min = false;
		res->has_damped_max = false;
	}

	return true;
}

bool handle_knob_set_config(const KnobConfig *req, KnobConfig *res)
{
	if (!knob) {
		return false;
	}

	if (req->demo) {
		knob_set_encoder_report(knob, false);
		knob_set_mode(knob, (enum knob_mode)req->mode);
		if (req->mode == KnobConfig_Mode_DAMPED && req->has_damped_min &&
		    req->has_damped_max) {
			knob_set_position_limit(knob, req->damped_min, req->damped_max);
		}
	} else {
		knob_set_mode(knob, KNOB_ENCODER);
		knob_set_encoder_report(knob, true);
	}

	return handle_knob_get_config(res);
}

static int handler_knob_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	knob = device_get_binding("KNOB");
	if (!knob) {
		return -EIO;
	}

	motor = device_get_binding("MOTOR");
	if (!motor) {
		return -EIO;
	}

	return 0;
}

SYS_INIT(handler_knob_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
