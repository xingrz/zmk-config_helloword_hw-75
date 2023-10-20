/*
 * Copyright (c) 2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_knob_profile_switch

#include <zephyr/device.h>
#include <zephyr/kernel.h>

#include <knob/math.h>
#include <knob/drivers/knob.h>
#include <knob/drivers/motor.h>
#include <knob/drivers/profile.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(knob_switch, CONFIG_ZMK_LOG_LEVEL);

struct knob_switch_config {
	KNOB_PROFILE_CFG_ROM;
	uint32_t on_off_distance_deg;
};

struct knob_switch_data {
	float center;
	float on;
	float on_2;
	float off;
	float off_2;
	bool state;
	bool last_report;
};

static int knob_switch_enable(const struct device *dev)
{
	const struct knob_switch_config *cfg = dev->config;
	struct knob_switch_data *data = dev->data;

	motor_set_torque_limit(cfg->motor, KNOB_PROFILE_TORQUE_LIMIT);

#if KNOB_PROFILE_HAS_VELOCITY_PID
	motor_set_velocity_pid(cfg->motor, KNOB_PROFILE_VELOCITY_PID);
#endif /* KNOB_PROFILE_HAS_VELOCITY_PID */

#if KNOB_PROFILE_HAS_ANGLE_PID
	motor_set_angle_pid(cfg->motor, KNOB_PROFILE_ANGLE_PID);
#endif /* KNOB_PROFILE_HAS_ANGLE_PID */

	data->center = deg_to_rad(180);
	data->off = data->center + deg_to_rad(cfg->on_off_distance_deg) * 0.5f;
	data->off_2 = data->center + deg_to_rad(cfg->on_off_distance_deg) * 0.25f;
	data->on = data->center - deg_to_rad(cfg->on_off_distance_deg) * 0.5f;
	data->on_2 = data->center - deg_to_rad(cfg->on_off_distance_deg) * 0.25f;

	data->state = false;

	return 0;
}

static int knob_switch_update_params(const struct device *dev, struct knob_params params)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(params);

	return 0;
}

static int knob_switch_tick(const struct device *dev, struct motor_control *mc)
{
	const struct knob_switch_config *cfg = dev->config;
	struct knob_switch_data *data = dev->data;

	mc->mode = ANGLE;

	float p = knob_get_position(cfg->knob);
	if (p < data->on_2) {
		mc->target = data->on;
		data->state = true;
	} else if (p > data->off_2) {
		mc->target = data->off;
		data->state = false;
	} else {
		mc->target = data->center + (p - data->center) * 2.0f;
	}

	return 0;
}

static int knob_switch_report(const struct device *dev, int32_t *val)
{
	struct knob_switch_data *data = dev->data;

	if (data->last_report == data->state) {
		return 0;
	}

	*val = data->state ? -1 : 1;

	data->last_report = data->state;

	return 0;
}

static int knob_switch_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static const struct knob_profile_api knob_switch_api = {
	.enable = knob_switch_enable,
	.update_params = knob_switch_update_params,
	.tick = knob_switch_tick,
	.report = knob_switch_report,
};

static struct knob_switch_data knob_switch_data;

static const struct knob_switch_config knob_switch_cfg = {
	.on_off_distance_deg = DT_INST_PROP(0, on_off_distance_deg), KNOB_PROFILE_CFG_INIT};

DEVICE_DT_INST_DEFINE(0, knob_switch_init, NULL, &knob_switch_data, &knob_switch_cfg, POST_KERNEL,
		      CONFIG_SENSOR_INIT_PRIORITY, &knob_switch_api);
