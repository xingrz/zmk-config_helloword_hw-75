/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#include "handler.h"
#include "usb_comm.pb.h"

#include <device.h>
#include <knob/drivers/knob.h>
#include <knob/drivers/motor.h>

#include <knob_app.h>

static const struct device *knob;
static const struct device *motor;

static struct motor_state state = {};

static bool handle_motor_get_state(const usb_comm_MessageH2D *h2d, usb_comm_MessageD2H *d2h,
				   const void *bytes, uint32_t bytes_len)
{
	usb_comm_MotorState *res = &d2h->payload.motor_state;

	if (!motor) {
		return false;
	}

	motor_inspect(motor, &state);

	res->timestamp = state.timestamp;
	res->control_mode = (usb_comm_MotorState_ControlMode)state.control_mode;
	res->current_angle = state.current_angle;
	res->current_velocity = state.current_velocity;
	res->target_angle = state.target_angle;
	res->target_velocity = state.target_velocity;
	res->target_voltage = state.target_voltage;

	return true;
}

USB_COMM_HANDLER_DEFINE(usb_comm_Action_MOTOR_GET_STATE, usb_comm_MessageD2H_motor_state_tag,
			handle_motor_get_state);

static bool handle_knob_get_config(const usb_comm_MessageH2D *h2d, usb_comm_MessageD2H *d2h,
				   const void *bytes, uint32_t bytes_len)
{
	usb_comm_KnobConfig *res = &d2h->payload.knob_config;

	if (!knob) {
		return false;
	}

	res->demo = knob_app_get_demo();
	res->mode = (usb_comm_KnobConfig_Mode)knob_get_mode(knob);

	return true;
}

USB_COMM_HANDLER_DEFINE(usb_comm_Action_KNOB_GET_CONFIG, usb_comm_MessageD2H_knob_config_tag,
			handle_knob_get_config);

static bool handle_knob_set_config(const usb_comm_MessageH2D *h2d, usb_comm_MessageD2H *d2h,
				   const void *bytes, uint32_t bytes_len)
{
	const usb_comm_KnobConfig *req = &h2d->payload.knob_config;

	if (!knob) {
		return false;
	}

	if (req->demo) {
		knob_app_set_demo(true);
		knob_set_mode(knob, (enum knob_mode)req->mode);
	} else {
		knob_app_set_demo(false);
	}

	return handle_knob_get_config(h2d, d2h, NULL, 0);
}

USB_COMM_HANDLER_DEFINE(usb_comm_Action_KNOB_SET_CONFIG, usb_comm_MessageD2H_knob_config_tag,
			handle_knob_set_config);

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
