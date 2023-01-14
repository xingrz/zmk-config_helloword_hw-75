/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_knob

#include <device.h>
#include <drivers/sensor.h>
#include <kernel.h>
#include <logging/log.h>

#include <knob/math.h>
#include <knob/motor.h>
#include <knob/encoder_state.h>
#include <knob/drivers/knob.h>

LOG_MODULE_REGISTER(knob, CONFIG_ZMK_LOG_LEVEL);

struct knob_data {
	const struct device *dev;
	int32_t delta;

	int last_pos;

	sensor_trigger_handler_t handler;
	const struct sensor_trigger *trigger;

	K_THREAD_STACK_MEMBER(thread_stack, CONFIG_KNOB_THREAD_STACK_SIZE);
	struct k_thread thread;

	struct motor motor;

	enum knob_mode mode;

	float position_min;
	float position_max;

	bool encoder_report;
	int encoder_ppr;

	float last_angle;
	float last_velocity;
};

struct knob_config {
	const struct device *inverter;
	const struct device *encoder;
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

	switch (data->mode) {
	case KNOB_INERTIA: {
		float v = knob_get_velocity(dev);
		if (v > 1 || v < -1) {
			if (fabsf(v - data->last_velocity) > 0.3) {
				data->motor.target = v;
			}
		} else {
			data->motor.target = 0.0f;
		}
		data->last_velocity = v;
	} break;
	case KNOB_ENCODER: {
		float p = knob_get_position(dev);
		if (p - data->last_angle > PI / (float)data->encoder_ppr) {
			data->motor.target += PI2 / (float)data->encoder_ppr;
			data->last_angle = data->motor.target;
		} else if (p - data->last_angle < -PI / (float)data->encoder_ppr) {
			data->motor.target -= PI2 / (float)data->encoder_ppr;
			data->last_angle = data->motor.target;
		}
	} break;
	case KNOB_DAMPED: {
		if (data->position_min == data->position_max) {
			break;
		}

		float p = knob_get_position(dev);
		if (p > data->position_max) {
			data->motor.control_mode = ANGLE;
			data->motor.target = data->position_max;
		} else if (p < data->position_min) {
			data->motor.control_mode = ANGLE;
			data->motor.target = data->position_min;
		} else {
			data->motor.control_mode = VELOCITY;
			data->motor.target = 0;
		}
	} break;
	case KNOB_DISABLE:
	case KNOB_SPRING:
	case KNOB_SPIN:
		break;
	}

	motor_tick(&data->motor);
}

void knob_set_mode(const struct device *dev, enum knob_mode mode)
{
	struct knob_data *data = dev->data;

	data->mode = mode;

	data->last_angle = knob_get_position(dev);
	data->last_velocity = knob_get_velocity(dev);

	switch (mode) {
	case KNOB_DISABLE:
		motor_set_enable(&data->motor, false);
		break;
	case KNOB_INERTIA: {
		motor_set_enable(&data->motor, true);
		motor_set_torque_limit(&data->motor, 0.5f);
		data->motor.control_mode = VELOCITY;
		pid_set(&data->motor.pid_velocity, 0.1f, 0.0f, 0.0f);
		pid_set(&data->motor.pid_angle, 20.0f, 0.0f, 0.7f);
		data->motor.target = 0.0f;
	} break;
	case KNOB_ENCODER: {
		motor_set_enable(&data->motor, true);
		motor_set_torque_limit(&data->motor, 0.2f);
		data->motor.control_mode = ANGLE;
		pid_set(&data->motor.pid_velocity, 0.1f, 0.0f, 0.0f);
		pid_set(&data->motor.pid_angle, 100.0f, 0.0f, 3.5f);
		data->motor.target = 4.2f;
		data->last_angle = 4.2f;
	} break;
	case KNOB_SPRING: {
		motor_set_enable(&data->motor, true);
		motor_set_torque_limit(&data->motor, 1.5f);
		data->motor.control_mode = ANGLE;
		pid_set(&data->motor.pid_velocity, 0.1f, 0.0f, 0.0f);
		pid_set(&data->motor.pid_angle, 100.0f, 0.0f, 3.5f);
		data->motor.target = 4.2f;
	} break;
	case KNOB_DAMPED: {
		motor_set_enable(&data->motor, true);
		motor_set_torque_limit(&data->motor, 1.5f);
		data->motor.control_mode = VELOCITY;
		pid_set(&data->motor.pid_velocity, 0.1f, 0.0f, 0.0f);
		data->motor.target = 0.0f;
	} break;
	case KNOB_SPIN: {
		motor_set_enable(&data->motor, true);
		motor_set_torque_limit(&data->motor, 1.5f);
		data->motor.control_mode = VELOCITY;
		pid_set(&data->motor.pid_velocity, 0.3f, 0.0f, 0.0f);
		data->motor.target = 20.0f;
	} break;
	}
}

void knob_set_enable(const struct device *dev, bool enable)
{
	struct knob_data *data = dev->data;
	motor_set_enable(&data->motor, enable);
}

void knob_set_encoder_report(const struct device *dev, bool report)
{
	struct knob_data *data = dev->data;
	data->encoder_report = report;
}

void knob_set_position_limit(const struct device *dev, float min, float max)
{
	struct knob_data *data = dev->data;
	data->position_min = min;
	data->position_max = max;
}

float knob_get_position(const struct device *dev)
{
	struct knob_data *data = dev->data;
	return motor_get_estimate_angle(&data->motor);
}

float knob_get_velocity(const struct device *dev)
{
	struct knob_data *data = dev->data;
	return motor_get_estimate_velocity(&data->motor);
}

int knob_get_encoder_position(const struct device *dev)
{
	struct knob_data *data = dev->data;
	return lroundf(knob_get_position(dev) / (PI2 / (float)data->encoder_ppr)) *
	       data->motor.encoder_dir;
}

void knob_calibrate_set(const struct device *dev, float zero_offset, int direction)
{
	struct knob_data *data = dev->data;
	data->motor.zero_offset = zero_offset;
	data->motor.encoder_dir = (enum encoder_direction)direction;
}

void knob_calibrate_get(const struct device *dev, float *zero_offset, int *direction)
{
	struct knob_data *data = dev->data;
	*zero_offset = data->motor.zero_offset;
	*direction = data->motor.encoder_dir;
}

bool knob_calibrate_auto(const struct device *dev)
{
	struct knob_data *data = dev->data;

	if (motor_calibrate_auto(&data->motor)) {
		LOG_INF("Motor calibrated, zero: %f (%f deg), direction: %d",
			data->motor.zero_offset, rad_to_deg(data->motor.zero_offset),
			data->motor.encoder_dir);
		return true;
	} else {
		LOG_ERR("Motor calibration failed");
		motor_set_enable(&data->motor, false);
		return false;
	}
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

	data->dev = dev;

	if (!device_is_ready(config->inverter)) {
		LOG_ERR("%s: Inverter device is not ready: %s", dev->name, config->inverter->name);
		return -ENODEV;
	}

	if (!device_is_ready(config->encoder)) {
		LOG_ERR("%s: Encoder device is not ready: %s", dev->name, config->encoder->name);
		return -ENODEV;
	}

	motor_init(&data->motor, config->encoder, config->inverter, 7);
	data->motor.control_mode = TORQUE;
	data->motor.voltage_limit = 1.5f;
	data->motor.velocity_limit = 100.0f;
	pid_set(&data->motor.pid_velocity, 0.1f, 0.0f, 0.0f);
	pid_set(&data->motor.pid_angle, 80.0f, 0.0f, 0.7f);

#if 1
	if (!knob_calibrate_auto(dev)) {
		return -EIO;
	}

	knob_set_enable(dev, true);
	knob_set_encoder_report(dev, true);
	knob_set_mode(dev, KNOB_ENCODER);
#endif

	k_thread_create(&data->thread, data->thread_stack, CONFIG_KNOB_THREAD_STACK_SIZE,
			(k_thread_entry_t)knob_thread, (void *)dev, 0, NULL,
			K_PRIO_COOP(CONFIG_KNOB_THREAD_PRIORITY), 0, K_NO_WAIT);

	return 0;
}

#define KNOB_INST(n)                                                                               \
	struct knob_data knob_data_##n = {                                                         \
		.mode = KNOB_DISABLE,                                                              \
		.position_min = 3.3f,                                                              \
		.position_max = 5.1f,                                                              \
		.encoder_report = true,                                                            \
		.encoder_ppr = DT_INST_PROP_OR(n, ppr, 24),                                        \
	};                                                                                         \
                                                                                                   \
	const struct knob_config knob_config_##n = {                                               \
		.inverter = DEVICE_DT_GET(DT_INST_PHANDLE(n, inverter)),                           \
		.encoder = DEVICE_DT_GET(DT_INST_PHANDLE(n, encoder)),                             \
		.tick_interval_us = DT_INST_PROP_OR(n, tick_interval_us, 200),                     \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, knob_init, NULL, &knob_data_##n, &knob_config_##n, POST_KERNEL,   \
			      CONFIG_SENSOR_INIT_PRIORITY, &knob_driver_api);

DT_INST_FOREACH_STATUS_OKAY(KNOB_INST)
