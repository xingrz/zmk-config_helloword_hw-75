/*
 * Copyright (c) 2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_knob_profile_ratchet

#include <zephyr/device.h>
#include <zephyr/kernel.h>

#include <knob/math.h>
#include <knob/drivers/knob.h>
#include <knob/drivers/motor.h>
#include <knob/drivers/profile.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(knob_ratchet, CONFIG_ZMK_LOG_LEVEL);

struct knob_ratchet_config {
	KNOB_PROFILE_CFG_ROM;
};

struct knob_ratchet_data {
	float last_angle;
};

static int knob_ratchet_enable(const struct device *dev)
{
	const struct knob_ratchet_config *cfg = dev->config;
	struct knob_ratchet_data *data = dev->data;

	motor_set_torque_limit(cfg->motor, KNOB_PROFILE_TORQUE_LIMIT);

#if KNOB_PROFILE_HAS_VELOCITY_PID
	motor_set_velocity_pid(cfg->motor, KNOB_PROFILE_VELOCITY_PID);
#endif /* KNOB_PROFILE_HAS_VELOCITY_PID */

#if KNOB_PROFILE_HAS_ANGLE_PID
	motor_set_angle_pid(cfg->motor, KNOB_PROFILE_ANGLE_PID);
#endif /* KNOB_PROFILE_HAS_ANGLE_PID */

	data->last_angle = knob_get_position(cfg->knob);

	return 0;
}

static int knob_ratchet_update_params(const struct device *dev, struct knob_params params)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(params);

	return 0;
}

static int knob_ratchet_tick(const struct device *dev, struct motor_control *mc)
{
	const struct knob_ratchet_config *cfg = dev->config;
	struct knob_ratchet_data *data = dev->data;

	mc->mode = ANGLE;
	mc->target = data->last_angle;

	float dp = knob_get_position(cfg->knob) - data->last_angle;
	float rpp = PI2 / 12;
	if (dp > rpp) {
		data->last_angle += rpp;
	}

	return 0;
}

static int knob_ratchet_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static const struct knob_profile_api knob_ratchet_api = {
	.enable = knob_ratchet_enable,
	.update_params = knob_ratchet_update_params,
	.tick = knob_ratchet_tick,
};

static struct knob_ratchet_data knob_ratchet_data;

static const struct knob_ratchet_config knob_ratchet_cfg = { KNOB_PROFILE_CFG_INIT };

DEVICE_DT_INST_DEFINE(0, knob_ratchet_init, NULL, &knob_ratchet_data, &knob_ratchet_cfg,
		      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &knob_ratchet_api);
