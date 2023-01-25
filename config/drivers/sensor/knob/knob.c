/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_knob

#include <device.h>
#include <kernel.h>
#include <drivers/sensor.h>

#include <knob/math.h>
#include <knob/encoder_state.h>
#include <knob/drivers/motor.h>
#include <knob/drivers/knob.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(knob, CONFIG_ZMK_LOG_LEVEL);

struct knob_data {
	int32_t delta;

	int last_pos;

	sensor_trigger_handler_t handler;
	const struct sensor_trigger *trigger;

	K_THREAD_STACK_MEMBER(thread_stack, CONFIG_KNOB_THREAD_STACK_SIZE);
	struct k_thread thread;

	struct motor_control *mc;

	enum knob_mode mode;

	float position_min;
	float position_max;

	bool encoder_report;
	float encoder_rpp;

	float last_angle;
	float last_velocity;
	float max_velocity;
};

struct knob_config {
	const struct device *motor;
	uint32_t tick_interval_us;
};

static int knob_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			    sensor_trigger_handler_t handler)
{
	struct knob_data *data = dev->data;
	data->trigger = trig;
	data->handler = handler;
	return 0;
}

static int knob_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(chan);
	return 0;
}

static int knob_channel_get(const struct device *dev, enum sensor_channel chan,
			    struct sensor_value *val)
{
	struct knob_data *data = dev->data;

	if (chan != SENSOR_CHAN_ROTATION) {
		return -ENOTSUP;
	}

	val->val1 = data->delta;
	val->val2 = 0;

	return 0;
}

static void knob_tick(const struct device *dev)
{
	struct knob_data *data = dev->data;
	const struct knob_config *config = dev->config;

	struct motor_control *mc = data->mc;

	switch (data->mode) {
	case KNOB_INERTIA: {
		float v = knob_get_velocity(dev);
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
	} break;
	case KNOB_ENCODER: {
		float dp = knob_get_position(dev) - data->last_angle;
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
			}
		} else if (dp < 0) {
			if (dp > -rpp_4) {
				mc->target = data->last_angle - dp;
			} else {
				mc->target = data->last_angle - rpp_2 + (rpp_2 + dp) * 3.0f;
			}
			if (dp <= -rpp_2) {
				data->last_angle -= rpp;
			}
		}
	} break;
	case KNOB_DAMPED: {
		if (data->position_min == data->position_max) {
			break;
		}

		float p = knob_get_position(dev);
		if (p > data->position_max) {
			mc->mode = ANGLE;
			mc->target = data->position_max;
		} else if (p < data->position_min) {
			mc->mode = ANGLE;
			mc->target = data->position_min;
		} else {
			mc->mode = VELOCITY;
			mc->target = 0.0f;
		}
	} break;
	case KNOB_RATCHET: {
		float dp = knob_get_position(dev) - data->last_angle;
		float rpp = PI2 / 12;
		if (dp < 0) {
			mc->target = data->last_angle;
		} else if (dp < rpp) {
			mc->target = data->last_angle + dp - dp * 0.5f;
		} else {
			data->last_angle += rpp;
		}
	} break;
	case KNOB_DISABLE:
	case KNOB_SPRING:
	case KNOB_SPIN:
		break;
	}

	motor_tick(config->motor);
}

void knob_set_mode(const struct device *dev, enum knob_mode mode)
{
	struct knob_data *data = dev->data;
	const struct knob_config *config = dev->config;

	struct motor_control *mc = data->mc;

	data->mode = mode;

	data->last_angle = knob_get_position(dev);
	data->last_velocity = knob_get_velocity(dev);
	data->max_velocity = 0.0f;

	motor_reset_rotation_count(config->motor);

	switch (mode) {
	case KNOB_DISABLE:
		motor_set_enable(config->motor, false);
		break;
	case KNOB_INERTIA: {
		motor_set_enable(config->motor, true);
		motor_set_torque_limit(config->motor, 1.5f);
		mc->mode = VELOCITY;
		motor_set_velocity_pid(config->motor, 0.3f, 0.0f, 0.0f);
		motor_set_angle_pid(config->motor, 20.0f, 0.0f, 0.7f);
		mc->target = 0.0f;
	} break;
	case KNOB_ENCODER: {
		motor_set_enable(config->motor, true);
		motor_set_torque_limit(config->motor, 0.3f);
		mc->mode = ANGLE;
		motor_set_velocity_pid(config->motor, 0.02f, 0.0f, 0.0f);
		motor_set_angle_pid(config->motor, 100.0f, 0.0f, 3.5f);
		mc->target = 0.0f;
		data->last_angle = 0.0f;
	} break;
	case KNOB_SPRING: {
		motor_set_enable(config->motor, true);
		motor_set_torque_limit(config->motor, 1.5f);
		mc->mode = ANGLE;
		motor_set_velocity_pid(config->motor, 0.05f, 0.0f, 0.0f);
		motor_set_angle_pid(config->motor, 100.0f, 0.0f, 3.5f);
		mc->target = deg_to_rad(240);
	} break;
	case KNOB_DAMPED: {
		motor_set_enable(config->motor, true);
		motor_set_torque_limit(config->motor, 1.5f);
		mc->mode = VELOCITY;
		motor_set_velocity_pid(config->motor, 0.05f, 0.0f, 0.0f);
		motor_set_angle_pid(config->motor, 100.0f, 0.0f, 3.5f);
		mc->target = 0.0f;
	} break;
	case KNOB_SPIN: {
		motor_set_enable(config->motor, true);
		motor_set_torque_limit(config->motor, 1.5f);
		mc->mode = VELOCITY;
		motor_set_velocity_pid(config->motor, 0.3f, 0.0f, 0.0f);
		mc->target = 20.0f;
	} break;
	case KNOB_RATCHET: {
		motor_set_enable(config->motor, true);
		motor_set_torque_limit(config->motor, 2.5f);
		mc->mode = ANGLE;
		motor_set_velocity_pid(config->motor, 0.05f, 0.0f, 0.0f);
		motor_set_angle_pid(config->motor, 100.0f, 0.0f, 3.5f);
		mc->target = 0.0f;
		data->last_angle = 0.0f;
	} break;
	}
}

enum knob_mode knob_get_mode(const struct device *dev)
{
	struct knob_data *data = dev->data;
	return data->mode;
}

void knob_set_enable(const struct device *dev, bool enable)
{
	struct knob_data *data = dev->data;
	const struct knob_config *config = dev->config;
	motor_set_enable(config->motor, enable && data->mode != KNOB_DISABLE);
}

void knob_set_encoder_report(const struct device *dev, bool report)
{
	struct knob_data *data = dev->data;
	if (report) {
		data->last_pos = knob_get_encoder_position(dev);
	}
	data->encoder_report = report;
}

bool knob_get_encoder_report(const struct device *dev)
{
	struct knob_data *data = dev->data;
	return data->encoder_report;
}

void knob_set_position_limit(const struct device *dev, float min, float max)
{
	struct knob_data *data = dev->data;
	data->position_min = min;
	data->position_max = max;
}

void knob_get_position_limit(const struct device *dev, float *min, float *max)
{
	struct knob_data *data = dev->data;
	*min = data->position_min;
	*max = data->position_max;
}

float knob_get_position(const struct device *dev)
{
	const struct knob_config *config = dev->config;
	return motor_get_estimate_angle(config->motor);
}

float knob_get_velocity(const struct device *dev)
{
	const struct knob_config *config = dev->config;
	return motor_get_estimate_velocity(config->motor);
}

int knob_get_encoder_position(const struct device *dev)
{
	struct knob_data *data = dev->data;
	const struct knob_config *config = dev->config;
	return lroundf(knob_get_position(dev) / data->encoder_rpp) *
	       motor_get_direction(config->motor);
}

static inline void knob_fires(const struct device *dev, int32_t delta)
{
	struct knob_data *data = dev->data;

	data->delta = delta;
	if (data->handler != NULL) {
		data->handler(dev, data->trigger);
	}
}

static void knob_thread(void *p1, void *p2, void *p3)
{
	const struct device *dev = (const struct device *)p1;
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct knob_data *data = dev->data;
	const struct knob_config *config = dev->config;

	int pos, pos_delta;

	while (1) {
		knob_tick(dev);

		if (data->encoder_report) {
			pos = knob_get_encoder_position(dev);
			pos_delta = pos - data->last_pos;
			data->last_pos = pos;

			if (pos_delta > 0) {
				knob_fires(dev, 1);
			} else if (pos_delta < 0) {
				knob_fires(dev, -1);
			}
		}

		k_usleep(config->tick_interval_us);
	}
}

static const struct sensor_driver_api knob_driver_api = {
	.trigger_set = knob_trigger_set,
	.sample_fetch = knob_sample_fetch,
	.channel_get = knob_channel_get,
};

int knob_init(const struct device *dev)
{
	struct knob_data *data = dev->data;
	const struct knob_config *config = dev->config;

	if (!device_is_ready(config->motor)) {
		LOG_ERR("%s: Motor device is not ready: %s", dev->name, config->motor->name);
		return -ENODEV;
	}

	data->mc = motor_get_control(config->motor);

	k_thread_create(&data->thread, data->thread_stack, CONFIG_KNOB_THREAD_STACK_SIZE,
			(k_thread_entry_t)knob_thread, (void *)dev, 0, NULL,
			K_PRIO_COOP(CONFIG_KNOB_THREAD_PRIORITY), 0, K_NO_WAIT);

	return 0;
}

#define KNOB_INST(n)                                                                               \
	struct knob_data knob_data_##n = {                                                         \
		.mode = KNOB_DISABLE,                                                              \
		.position_min = deg_to_rad(190),                                                   \
		.position_max = deg_to_rad(290),                                                   \
		.encoder_report = false,                                                           \
		.encoder_rpp = PI2 / (float)DT_INST_PROP(n, ppr),                                  \
	};                                                                                         \
                                                                                                   \
	const struct knob_config knob_config_##n = {                                               \
		.motor = DEVICE_DT_GET(DT_INST_PHANDLE(n, motor)),                                 \
		.tick_interval_us = DT_INST_PROP_OR(n, tick_interval_us, 200),                     \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, knob_init, NULL, &knob_data_##n, &knob_config_##n, POST_KERNEL,   \
			      CONFIG_SENSOR_INIT_PRIORITY, &knob_driver_api);

DT_INST_FOREACH_STATUS_OKAY(KNOB_INST)
