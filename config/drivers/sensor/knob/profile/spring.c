/*
 * Copyright (c) 2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_knob_profile_spring

#include <zephyr/device.h>
#include <zephyr/kernel.h>

#include <knob/math.h>
#include <knob/drivers/knob.h>
#include <knob/drivers/motor.h>
#include <knob/drivers/profile.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(knob_spring, CONFIG_ZMK_LOG_LEVEL);

struct knob_spring_config {
	KNOB_PROFILE_CFG_ROM;
	uint32_t minimal_movement_deg;
};

struct knob_spring_data {
	float center;
	float up;
	float down;
	int32_t value;
	int32_t last_report;
};

static int knob_spring_enable(const struct device *dev)
{
	const struct knob_spring_config *cfg = dev->config;
	struct knob_spring_data *data = dev->data;

	motor_set_torque_limit(cfg->motor, KNOB_PROFILE_TORQUE_LIMIT);

#if KNOB_PROFILE_HAS_VELOCITY_PID
	motor_set_velocity_pid(cfg->motor, KNOB_PROFILE_VELOCITY_PID);
#endif /* KNOB_PROFILE_HAS_VELOCITY_PID */

#if KNOB_PROFILE_HAS_ANGLE_PID
	motor_set_angle_pid(cfg->motor, KNOB_PROFILE_ANGLE_PID);
#endif /* KNOB_PROFILE_HAS_ANGLE_PID */

	data->center = deg_to_rad(180);
	data->up = data->center - deg_to_rad(cfg->minimal_movement_deg);
	data->down = data->center + deg_to_rad(cfg->minimal_movement_deg);

	return 0;
}

static int knob_spring_update_params(const struct device *dev, struct knob_params params)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(params);

	return 0;
}

static int knob_spring_tick(const struct device *dev, struct motor_control *mc)
{
	const struct knob_spring_config *cfg = dev->config;
	struct knob_spring_data *data = dev->data;
	ARG_UNUSED(mc);

	float p = knob_get_position(cfg->knob);
	if (p < data->up) {
		data->value = 1;
	} else if (p > data->down) {
		data->value = -1;
	} else {
		data->value = 0;
	}

	mc->mode = ANGLE;
	mc->target = data->center;

	return 0;
}

static int knob_spring_report(const struct device *dev, int32_t *val)
{
	struct knob_spring_data *data = dev->data;

	if (data->last_report == data->value) {
		return 0;
	}

	*val = data->value;

	data->last_report = data->value;

	return 0;
}

static int knob_spring_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static const struct knob_profile_api knob_spring_api = {
	.enable = knob_spring_enable,
	.update_params = knob_spring_update_params,
	.tick = knob_spring_tick,
	.report = knob_spring_report,
};

static struct knob_spring_data knob_spring_data;

static const struct knob_spring_config knob_spring_cfg = {
	.minimal_movement_deg = DT_INST_PROP(0, minimal_movement_deg), KNOB_PROFILE_CFG_INIT};

DEVICE_DT_INST_DEFINE(0, knob_spring_init, NULL, &knob_spring_data, &knob_spring_cfg, POST_KERNEL,
		      CONFIG_SENSOR_INIT_PRIORITY, &knob_spring_api);
