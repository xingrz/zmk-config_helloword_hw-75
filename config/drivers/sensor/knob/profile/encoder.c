/*
 * Copyright (c) 2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_knob_profile_encoder

#include <zephyr/device.h>
#include <zephyr/kernel.h>

#include <knob/math.h>
#include <knob/drivers/knob.h>
#include <knob/drivers/motor.h>
#include <knob/drivers/profile.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(knob_encoder, CONFIG_ZMK_LOG_LEVEL);

struct knob_encoder_config {
	KNOB_PROFILE_CFG_ROM;
};

struct knob_encoder_data {
	float encoder_rpp;
	float last_angle;
	int32_t pulses;
	int32_t reported_pulses;
};

static int knob_encoder_enable(const struct device *dev)
{
	const struct knob_encoder_config *cfg = dev->config;
	struct knob_encoder_data *data = dev->data;

	motor_set_torque_limit(cfg->motor, KNOB_PROFILE_TORQUE_LIMIT);

#if KNOB_PROFILE_HAS_VELOCITY_PID
	motor_set_velocity_pid(cfg->motor, KNOB_PROFILE_VELOCITY_PID);
#endif /* KNOB_PROFILE_HAS_VELOCITY_PID */

#if KNOB_PROFILE_HAS_ANGLE_PID
	motor_set_angle_pid(cfg->motor, KNOB_PROFILE_ANGLE_PID);
#endif /* KNOB_PROFILE_HAS_ANGLE_PID */

	data->last_angle = knob_get_position(cfg->knob);
	data->pulses = 0;
	data->reported_pulses = 0;

	return 0;
}

static int knob_encoder_update_params(const struct device *dev, struct knob_params params)
{
	struct knob_encoder_data *data = dev->data;

	data->encoder_rpp = PI2 / (float)params.ppr;

	return 0;
}

static int knob_encoder_tick(const struct device *dev, struct motor_control *mc)
{
	const struct knob_encoder_config *cfg = dev->config;
	struct knob_encoder_data *data = dev->data;

	mc->mode = ANGLE;

	float dp = knob_get_position(cfg->knob) - data->last_angle;
	float rpp = data->encoder_rpp;
	float rpp_4 = rpp / 4.0f;
	float rpp_2 = rpp / 2.0f;
	if (dp == 0) {
		mc->target = data->last_angle;
	} else if (dp > 0) {
		if (dp < rpp_4) {
			mc->target = data->last_angle - dp;
		} else {
			mc->target = data->last_angle + rpp_2 - (rpp_2 - dp) * 3.0f;
		}
		if (dp >= rpp_2) {
			data->last_angle += rpp;
			data->pulses++;
		}
	} else if (dp < 0) {
		if (dp > -rpp_4) {
			mc->target = data->last_angle - dp;
		} else {
			mc->target = data->last_angle - rpp_2 + (rpp_2 + dp) * 3.0f;
		}
		if (dp <= -rpp_2) {
			data->last_angle -= rpp;
			data->pulses--;
		}
	}

	return 0;
}

static int knob_encoder_report(const struct device *dev, int32_t *val)
{
	struct knob_encoder_data *data = dev->data;

	if (data->pulses == data->reported_pulses) {
		return -EAGAIN;
	}

	if (data->pulses > data->reported_pulses) {
		*val = 1;
	} else {
		*val = -1;
	}

	data->reported_pulses = data->pulses;

	return 0;
}

static int knob_encoder_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static const struct knob_profile_api knob_encoder_api = {
	.enable = knob_encoder_enable,
	.update_params = knob_encoder_update_params,
	.tick = knob_encoder_tick,
	.report = knob_encoder_report,
};

static struct knob_encoder_data knob_encoder_data;

static const struct knob_encoder_config knob_encoder_cfg = { KNOB_PROFILE_CFG_INIT };

DEVICE_DT_INST_DEFINE(0, knob_encoder_init, NULL, &knob_encoder_data, &knob_encoder_cfg,
		      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &knob_encoder_api);
