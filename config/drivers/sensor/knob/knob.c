/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_knob

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>

#include <knob/math.h>
#include <knob/encoder_state.h>
#include <knob/drivers/motor.h>
#include <knob/drivers/knob.h>
#include <knob/drivers/profile.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(knob, CONFIG_ZMK_LOG_LEVEL);

struct knob_data {
	int32_t delta;

	sensor_trigger_handler_t handler;
	const struct sensor_trigger *trigger;

	K_THREAD_STACK_MEMBER(thread_stack, CONFIG_KNOB_THREAD_STACK_SIZE);
	struct k_thread thread;

	struct k_work report_work;

	struct motor_control *mc;

	enum knob_mode mode;
	const struct device *profile;

	struct knob_params params;

	float position_min;
	float position_max;

	bool encoder_report;
	int encoder_ppr;
};

struct knob_config {
	const struct device *motor;
	uint32_t tick_interval_us;
	const struct device **profiles;
	uint32_t profiles_cnt;
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

	/* Knob is physically mount reversed */
	val->val1 = 0;
	val->val2 = -data->delta;

	return 0;
}

void knob_set_mode(const struct device *dev, enum knob_mode mode)
{
	struct knob_data *data = dev->data;
	const struct knob_config *config = dev->config;

	data->mode = mode;

	if (mode >= 0 && mode < config->profiles_cnt) {
		data->profile = config->profiles[mode];
	} else {
		data->profile = NULL;
	}

	motor_reset_rotation_count(config->motor);

	if (mode == KNOB_DISABLE) {
		motor_set_enable(config->motor, false);
	} else {
		motor_set_enable(config->motor, true);
	}

	if (data->profile != NULL) {
		knob_profile_update_params(data->profile, data->params);
		knob_profile_enable(data->profile);
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
	data->encoder_report = report;
}

bool knob_get_encoder_report(const struct device *dev)
{
	struct knob_data *data = dev->data;
	return data->encoder_report;
}

void knob_set_encoder_ppr(const struct device *dev, int ppr)
{
	struct knob_data *data = dev->data;

	if (ppr > 0) {
		data->params.ppr = ppr;

		if (data->profile != NULL) {
			knob_profile_update_params(data->profile, data->params);
		}
	}
}

int knob_get_encoder_ppr(const struct device *dev)
{
	struct knob_data *data = dev->data;
	return data->params.ppr;
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

static void knob_report_work_handler(struct k_work *work)
{
	struct knob_data *data = CONTAINER_OF(work, struct knob_data, report_work);
	const struct device *dev = CONTAINER_OF(data, struct device, data);

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

	float p;
	bool limited = false;

	while (1) {
		if (data->profile != NULL) {
			limited = false;
			if (data->position_min != data->position_max) {
				p = knob_get_position(dev);
				if (p > data->position_max) {
					data->mc->mode = ANGLE;
					data->mc->target = data->position_max;
					limited = true;
				} else if (p < data->position_min) {
					data->mc->mode = ANGLE;
					data->mc->target = data->position_min;
					limited = true;
				}
			}
			if (!limited) {
				knob_profile_tick(data->profile, data->mc);
			}

			motor_tick(config->motor);

			data->delta = 0;
			if (data->encoder_report &&
			    knob_profile_report(data->profile, &data->delta) == 0 &&
			    data->delta != 0) {
				k_work_submit(&data->report_work);
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

	data->params.ppr = data->encoder_ppr;

	k_thread_create(&data->thread, data->thread_stack, CONFIG_KNOB_THREAD_STACK_SIZE,
			(k_thread_entry_t)knob_thread, (void *)dev, 0, NULL,
			K_PRIO_COOP(CONFIG_KNOB_THREAD_PRIORITY), 0, K_NO_WAIT);

	k_work_init(&data->report_work, knob_report_work_handler);

	return 0;
}

#define KNOB_PROFILE_ELEM(node_id) [DT_REG_ADDR(node_id)] = DEVICE_DT_GET(node_id),

#define KNOB_INST(n)                                                                               \
	static struct knob_data knob_data_##n = {                                                  \
		.mode = KNOB_DISABLE,                                                              \
		.position_min = 0,                                                                 \
		.position_max = 0,                                                                 \
		.encoder_report = false,                                                           \
		.encoder_ppr = DT_INST_PROP(n, ppr),                                               \
	};                                                                                         \
                                                                                                   \
	static const struct device *knob_profiles_##n[] = { DT_INST_FOREACH_CHILD_STATUS_OKAY(     \
		n, KNOB_PROFILE_ELEM) };                                                           \
                                                                                                   \
	static const struct knob_config knob_config_##n = {                                        \
		.motor = DEVICE_DT_GET(DT_INST_PHANDLE(n, motor)),                                 \
		.tick_interval_us = DT_INST_PROP_OR(n, tick_interval_us, 200),                     \
		.profiles = knob_profiles_##n,                                                     \
		.profiles_cnt = ARRAY_SIZE(knob_profiles_##n),                                     \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, knob_init, NULL, &knob_data_##n, &knob_config_##n, POST_KERNEL,   \
			      CONFIG_SENSOR_INIT_PRIORITY, &knob_driver_api);

DT_INST_FOREACH_STATUS_OKAY(KNOB_INST)
