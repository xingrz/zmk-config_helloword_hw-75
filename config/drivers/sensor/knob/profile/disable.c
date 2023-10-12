/*
 * Copyright (c) 2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_knob_profile_disable

#include <zephyr/device.h>
#include <zephyr/kernel.h>

#include <knob/math.h>
#include <knob/drivers/knob.h>
#include <knob/drivers/motor.h>
#include <knob/drivers/profile.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(knob_disable, CONFIG_ZMK_LOG_LEVEL);

struct knob_disable_config {
	KNOB_PROFILE_CFG_ROM;
};

struct knob_disable_data {
	float encoder_rpp;
	float last_angle;
	int32_t pulses;
	int32_t reported_pulses;
};

static int knob_disable_enable(const struct device *dev)
{
	const struct knob_disable_config *cfg = dev->config;
	struct knob_disable_data *data = dev->data;

	data->last_angle = knob_get_position(cfg->knob);
	data->pulses = 0;
	data->reported_pulses = 0;

	return 0;
}

static int knob_disable_update_params(const struct device *dev, struct knob_params params)
{
	struct knob_disable_data *data = dev->data;

	data->encoder_rpp = PI2 / (float)params.ppr;

	return 0;
}

static int knob_disable_tick(const struct device *dev, struct motor_control *mc)
{
	const struct knob_disable_config *cfg = dev->config;
	struct knob_disable_data *data = dev->data;
	ARG_UNUSED(mc);

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

	return 0;
}

static int knob_disable_report(const struct device *dev, int32_t *val)
{
	struct knob_disable_data *data = dev->data;

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

static int knob_disable_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static const struct knob_profile_api knob_disable_api = {
	.enable = knob_disable_enable,
	.update_params = knob_disable_update_params,
	.tick = knob_disable_tick,
	.report = knob_disable_report,
};

static struct knob_disable_data knob_disable_data;

static const struct knob_disable_config knob_disable_cfg = { KNOB_PROFILE_CFG_INIT };

DEVICE_DT_INST_DEFINE(0, knob_disable_init, NULL, &knob_disable_data, &knob_disable_cfg,
		      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &knob_disable_api);
