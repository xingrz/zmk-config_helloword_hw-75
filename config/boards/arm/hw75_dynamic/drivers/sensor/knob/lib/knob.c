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
#include <knob/drivers/encoder.h>

LOG_MODULE_REGISTER(knob, CONFIG_ZMK_LOG_LEVEL);

struct knob_data {
	const struct device *dev;
	int32_t delta;

	float last_radian;

	sensor_trigger_handler_t handler;
	const struct sensor_trigger *trigger;

	K_THREAD_STACK_MEMBER(thread_stack, CONFIG_KNOB_THREAD_STACK_SIZE);
	struct k_thread thread;
};

struct knob_config {
	const struct device *encoder;
	uint32_t tick_interval_us;
	float radian_per_pulse;
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

	float radian, radian_delta;
	float rpp = config->radian_per_pulse;

	while (1) {
		radian = encoder_get_radian(config->encoder);
		radian_delta = radian - data->last_radian;

		if (radian_delta < -PI) {
			radian_delta += PI2;
		} else if (radian_delta > PI) {
			radian_delta -= PI2;
		}

		if (radian_delta >= rpp) {
			data->last_radian = radian;
			knob_fires(dev, 1);
		} else if (radian_delta <= -rpp) {
			data->last_radian = radian;
			knob_fires(dev, -1);
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

	if (!device_is_ready(config->encoder)) {
		LOG_ERR("%s: Encoder device is not ready: %s", dev->name, config->encoder->name);
		return -ENODEV;
	}

	data->last_radian = encoder_get_radian(config->encoder);

	k_thread_create(&data->thread, data->thread_stack, CONFIG_KNOB_THREAD_STACK_SIZE,
			(k_thread_entry_t)knob_thread, (void *)dev, 0, NULL,
			K_PRIO_COOP(CONFIG_KNOB_THREAD_PRIORITY), 0, K_NO_WAIT);

	return 0;
}

#define KNOB_INST(n)                                                                               \
	struct knob_data knob_data_##n;                                                            \
	const struct knob_config knob_config_##n = {                                               \
		.encoder = DEVICE_DT_GET(DT_INST_PHANDLE(n, encoder)),                             \
		.tick_interval_us = DT_INST_PROP_OR(n, tick_interval_us, 10000),                   \
		.radian_per_pulse = PI2 / (float)DT_INST_PROP_OR(n, ppr, 24),                      \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, knob_init, NULL, &knob_data_##n, &knob_config_##n, POST_KERNEL,   \
			      CONFIG_SENSOR_INIT_PRIORITY, &knob_driver_api);

DT_INST_FOREACH_STATUS_OKAY(KNOB_INST)
