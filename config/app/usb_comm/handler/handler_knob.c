/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#include "handler.h"
#include "usb_comm.pb.h"

#include <zephyr/device.h>
#include <knob/drivers/knob.h>
#include <knob/drivers/motor.h>

#include <knob_app.h>

#include <pb_encode.h>

#define KNOB_NODE DT_ALIAS(knob)
#define MOTOR_NODE DT_PHANDLE(KNOB_NODE, motor)

static const struct device *knob = DEVICE_DT_GET(KNOB_NODE);
static const struct device *motor = DEVICE_DT_GET(MOTOR_NODE);

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

static bool write_string(pb_ostream_t *stream, const pb_field_t *field, void *const *arg)
{
	char *str = *arg;
	if (!pb_encode_tag_for_field(stream, field)) {
		return false;
	}
	return pb_encode_string(stream, (uint8_t *)str, strlen(str));
}

static bool write_prefs(pb_ostream_t *stream, const pb_field_t *field, void *const *arg)
{
	const struct knob_pref *prefs;
	const char **names;
	int layers = knob_app_get_prefs(&prefs, &names);
	if (!layers) {
		return false;
	}

	enum knob_mode mode = knob_get_mode(knob);
	int ppr = knob_get_encoder_ppr(knob);
	float torque_limit = motor_get_torque_limit(motor);

	usb_comm_KnobConfig_Pref pref = usb_comm_KnobConfig_Pref_init_zero;
	for (int i = 0; i < layers; i++) {
		if (!pb_encode_tag_for_field(stream, field)) {
			return false;
		}

		pref.layer_id = i;
		pref.layer_name.funcs.encode = write_string;
		pref.layer_name.arg = (void *)names[i];
		pref.active = prefs[i].active;
		pref.mode = (usb_comm_KnobConfig_Mode)(prefs[i].active ? prefs[i].mode : mode);
		pref.has_mode = true;
		pref.ppr = prefs[i].active ? prefs[i].ppr : ppr;
		pref.has_ppr = true;
		pref.torque_limit = prefs[i].active ? prefs[i].torque_limit : torque_limit;
		pref.has_torque_limit = true;

		if (!pb_encode_submessage(stream, usb_comm_KnobConfig_Pref_fields, &pref)) {
			return false;
		}
	}

	return true;
}

static bool handle_knob_get_config(const usb_comm_MessageH2D *h2d, usb_comm_MessageD2H *d2h,
				   const void *bytes, uint32_t bytes_len)
{
	usb_comm_KnobConfig *res = &d2h->payload.knob_config;

	if (!knob) {
		return false;
	}

	res->demo = knob_app_get_demo();
	res->mode = (usb_comm_KnobConfig_Mode)knob_get_mode(knob);
	res->prefs.funcs.encode = write_prefs;

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

static bool handle_knob_update_pref(const usb_comm_MessageH2D *h2d, usb_comm_MessageD2H *d2h,
				    const void *bytes, uint32_t bytes_len)
{
	const usb_comm_KnobConfig_Pref *req = &h2d->payload.knob_pref;
	usb_comm_KnobConfig_Pref *res = &d2h->payload.knob_pref;

	{
		struct knob_pref pref = {
			.active = req->active,
			.mode = req->has_mode ? (enum knob_mode)req->mode : KNOB_ENCODER,
			.ppr = req->has_ppr ? req->ppr : 0,
			.torque_limit = req->has_torque_limit ? req->torque_limit : 0,
		};
		knob_app_set_pref(req->layer_id, &pref);
	}

	{
		enum knob_mode mode = knob_get_mode(knob);
		int ppr = knob_get_encoder_ppr(knob);
		float torque_limit = motor_get_torque_limit(motor);

		const struct knob_pref *pref = knob_app_get_pref(req->layer_id);
		if (pref == NULL) {
			return false;
		}

		res->layer_id = req->layer_id;
		res->active = pref->active;
		res->mode = (usb_comm_KnobConfig_Mode)(pref->active ? pref->mode : mode);
		res->has_mode = true;
		res->ppr = pref->active ? pref->ppr : ppr;
		res->has_ppr = true;
		res->torque_limit = pref->active ? pref->torque_limit : torque_limit;
		res->has_torque_limit = true;
	}

	return true;
}

USB_COMM_HANDLER_DEFINE(usb_comm_Action_KNOB_UPDATE_PREF, usb_comm_MessageD2H_knob_pref_tag,
			handle_knob_update_pref);
