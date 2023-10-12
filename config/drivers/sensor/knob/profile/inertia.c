/*
 * Copyright (c) 2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_knob_profile_inertia

#include <zephyr/device.h>
#include <zephyr/kernel.h>

#include <knob/math.h>
#include <knob/drivers/knob.h>
#include <knob/drivers/motor.h>
#include <knob/drivers/profile.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(knob_inertia, CONFIG_ZMK_LOG_LEVEL);

struct knob_inertia_config {
	KNOB_PROFILE_CFG_ROM;
};

struct knob_inertia_data {
	float encoder_rpp;
	float last_angle;
	int32_t pulses;
	int32_t reported_pulses;

	float last_velocity;
	float max_velocity;
};

static int knob_inertia_enable(const struct device *dev)
{
	const struct knob_inertia_config *cfg = dev->config;
	struct knob_inertia_data *data = dev->data;

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

	data->last_velocity = knob_get_velocity(cfg->knob);
	data->max_velocity = 0.0f;

	return 0;
}

static int knob_inertia_update_params(const struct device *dev, struct knob_params params)
{
	struct knob_inertia_data *data = dev->data;

	data->encoder_rpp = PI2 / (float)params.ppr;

	return 0;
}

static int knob_inertia_tick(const struct device *dev, struct motor_control *mc)
{
	const struct knob_inertia_config *cfg = dev->config;
	struct knob_inertia_data *data = dev->data;

	mc->mode = VELOCITY;

	float dp = knob_get_position(cfg->knob) - data->last_angle;
	float rpp = data->encoder_rpp;
	float rpp_2 = rpp / 2.0f;

	if (dp >= rpp_2) {
		data->last_angle += rpp;
		data->pulses++;
	} else if (dp <= -rpp_2) {
		data->last_angle -= rpp;
		data->pulses--;
	}

	float v = knob_get_velocity(cfg->knob);
	float a = v - data->last_velocity;
	if (v == 0.0f) {
		mc->target = 0.0f;
		data->max_velocity = 0.0f;
	} else if (v > 0.0f) {
		if (a > 1.0f || v > data->max_velocity) {
			mc->target = v;
			data->max_velocity = v;
		} else if (a < -2.0f) {
			mc->target += a;
			if (mc->target < 1.0f) {
				mc->target = 0.0f;
				data->max_velocity = 0.0f;
			}
		} else {
			mc->target -= 0.001f;
		}
	} else if (v < 0.0f) {
		if (a < -1.0f || v < data->max_velocity) {
			mc->target = v;
			data->max_velocity = v;
		} else if (a > 2.0f) {
			mc->target += a;
			if (mc->target > -1.0f) {
				mc->target = 0.0f;
				data->max_velocity = 0.0f;
			}
		} else {
			mc->target += 0.001f;
		}
	}
	data->last_velocity = mc->target;

	return 0;
}

static int knob_inertia_report(const struct device *dev, int32_t *val)
{
	struct knob_inertia_data *data = dev->data;

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

static int knob_inertia_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static const struct knob_profile_api knob_inertia_api = {
	.enable = knob_inertia_enable,
	.update_params = knob_inertia_update_params,
	.tick = knob_inertia_tick,
	.report = knob_inertia_report,
};

static struct knob_inertia_data knob_inertia_data;

static const struct knob_inertia_config knob_inertia_cfg = { KNOB_PROFILE_CFG_INIT };

DEVICE_DT_INST_DEFINE(0, knob_inertia_init, NULL, &knob_inertia_data, &knob_inertia_cfg,
		      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &knob_inertia_api);
