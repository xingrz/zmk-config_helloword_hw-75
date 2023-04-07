/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_led_strip_remap

#include <zephyr/drivers/led_strip.h>
#include <zephyr/drivers/led_strip_remap.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct led_strip_remap_indicator {
	const char *label;
	uint32_t *led_indexes;
	uint32_t led_cnt;
};

struct led_strip_remap_indicator_state {
	bool active;
	struct led_rgb color;
};

struct led_strip_remap_data {
	struct led_rgb *pixels;
	struct led_rgb *output;
	struct led_strip_remap_indicator_state *indicators;
	struct k_mutex lock;
};

struct led_strip_remap_config {
	uint32_t chain_length;
	const struct device *led_strip;
	uint32_t led_strip_len;
	const uint32_t *map;
	uint32_t map_len;
	const struct led_strip_remap_indicator *indicators;
	uint32_t indicator_cnt;
};

static int led_strip_remap_apply(const struct device *dev)
{
	struct led_strip_remap_data *data = dev->data;
	const struct led_strip_remap_config *config = dev->config;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	for (uint32_t i = 0; i < config->map_len; i++) {
		memcpy(&data->output[i], &data->pixels[i], sizeof(struct led_rgb));
	}

	const struct led_strip_remap_indicator *indicator;
	struct led_strip_remap_indicator_state *indicator_state;
	for (uint32_t i = 0; i < config->indicator_cnt; i++) {
		indicator = &config->indicators[i];
		indicator_state = &data->indicators[i];

		if (!indicator_state->active) {
			continue;
		}

		for (uint32_t j = 0; j < indicator->led_cnt; j++) {
			memcpy(&data->output[indicator->led_indexes[j]], &indicator_state->color,
			       sizeof(struct led_rgb));
		}
	}

	ret = led_strip_update_rgb(config->led_strip, data->output, config->map_len);

	k_mutex_unlock(&data->lock);

	return ret;
}

static int led_strip_remap_update_rgb(const struct device *dev, struct led_rgb *pixels,
				      size_t num_pixels)
{
	struct led_strip_remap_data *data = dev->data;
	const struct led_strip_remap_config *config = dev->config;

	if (num_pixels > config->map_len) {
		num_pixels = config->map_len;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	for (uint32_t i = 0; i < num_pixels; i++) {
		memcpy(&data->pixels[config->map[i]], &pixels[i], sizeof(struct led_rgb));
	}

	k_mutex_unlock(&data->lock);

	return led_strip_remap_apply(dev);
}

static int led_strip_remap_update_channels(const struct device *dev, uint8_t *channels,
					   size_t num_channels)
{
	LOG_ERR("update_channels not implemented");
	return -ENOTSUP;
}

int led_strip_remap_set(const struct device *dev, const char *label, struct led_rgb *pixel)
{
	struct led_strip_remap_data *data = dev->data;
	const struct led_strip_remap_config *config = dev->config;

	k_mutex_lock(&data->lock, K_FOREVER);

	const struct led_strip_remap_indicator *indicator;
	struct led_strip_remap_indicator_state *indicator_state;
	for (uint32_t i = 0; i < config->indicator_cnt; i++) {
		indicator = &config->indicators[i];
		indicator_state = &data->indicators[i];

		if (strcmp(indicator->label, label) != 0) {
			continue;
		}

		memcpy(&indicator_state->color, pixel, sizeof(struct led_rgb));
		indicator_state->active = true;
	}

	k_mutex_unlock(&data->lock);

	return led_strip_remap_apply(dev);
}

int led_strip_remap_clear(const struct device *dev, const char *label)
{
	struct led_strip_remap_data *data = dev->data;
	const struct led_strip_remap_config *config = dev->config;

	k_mutex_lock(&data->lock, K_FOREVER);

	const struct led_strip_remap_indicator *indicator;
	struct led_strip_remap_indicator_state *indicator_state;
	for (uint32_t i = 0; i < config->indicator_cnt; i++) {
		indicator = &config->indicators[i];
		indicator_state = &data->indicators[i];

		if (strcmp(indicator->label, label) != 0) {
			continue;
		}

		indicator_state->active = false;
	}

	k_mutex_unlock(&data->lock);

	return led_strip_remap_apply(dev);
}

static int led_strip_remap_init(const struct device *dev)
{
	struct led_strip_remap_data *data = dev->data;
	const struct led_strip_remap_config *config = dev->config;

	k_mutex_init(&data->lock);

	if (config->chain_length != config->led_strip_len) {
		LOG_ERR("%s: chain-length (%d) should be the same with led-strip device %s (%d)",
			dev->name, config->chain_length, config->led_strip->name,
			config->led_strip_len);
		return -EINVAL;
	}

	if (config->map_len != config->chain_length) {
		LOG_ERR("%s: map (%d) should have the same length with chain-length (%d)",
			dev->name, config->map_len, config->chain_length);
		return -EINVAL;
	}

	for (uint32_t i = 0; i < config->map_len; i++) {
		uint32_t idx = config->map[i];
		if (idx >= config->led_strip_len) {
			LOG_ERR("%s: Map at index %d (%d) overflows chain-length of led-strip %s (%d)",
				dev->name, i, idx, config->led_strip->name, config->led_strip_len);
			return -EINVAL;
		}
	}

	const struct led_strip_remap_indicator *indicator;
	for (uint32_t i = 0; i < config->indicator_cnt; i++) {
		indicator = &config->indicators[i];
		for (uint32_t j = 0; j < indicator->led_cnt; j++) {
			if (indicator->led_indexes[j] >= config->led_strip_len) {
				LOG_ERR("%s: Index %d at indicator %s overflows chain-length of led-strip %s (%d)",
					dev->name, indicator->led_indexes[j], indicator->label,
					config->led_strip->name, config->led_strip_len);
				return -EINVAL;
			}
		}
	}

	return 0;
}

static const struct led_strip_driver_api led_strip_remap_api = {
	.update_rgb = led_strip_remap_update_rgb,
	.update_channels = led_strip_remap_update_channels,
};

#define LED_STRIP_REMAP_INDICATOR(node_id, n)                                                      \
	{                                                                                          \
		.label = DT_PROP(node_id, label),                                                  \
		.led_indexes = led_strip_remap_indicator_indexes_##n,                              \
		.led_cnt = DT_PROP_LEN(node_id, led_indexes),                                      \
	},

#define LED_STRIP_REMAP_INDICATOR_INDEXES(node_id, n)                                              \
	static uint32_t led_strip_remap_indicator_indexes_##n[] = DT_PROP(node_id, led_indexes);

#define LED_STRIP_REMAP_INDICATOR_STATE(node_id)                                                   \
	{                                                                                          \
		.active = false,                                                                   \
	},

#define LED_STRIP_REMAP_INIT(n)                                                                    \
	static struct led_rgb led_strip_remap_pixels_##n[DT_INST_PROP_LEN(n, map)] = { 0 };        \
	static struct led_rgb led_strip_remap_output_##n[DT_INST_PROP_LEN(n, map)] = { 0 };        \
                                                                                                   \
	static struct led_strip_remap_indicator_state led_strip_remap_indicator_states_##n[] = {   \
		DT_INST_FOREACH_CHILD(n, LED_STRIP_REMAP_INDICATOR_STATE)                          \
	};                                                                                         \
                                                                                                   \
	static struct led_strip_remap_data led_strip_remap_data_##n = {                            \
		.pixels = led_strip_remap_pixels_##n,                                              \
		.output = led_strip_remap_output_##n,                                              \
		.indicators = led_strip_remap_indicator_states_##n,                                \
	};                                                                                         \
                                                                                                   \
	static const uint32_t led_strip_remap_map_##n[] = DT_INST_PROP(n, map);                    \
                                                                                                   \
	DT_INST_FOREACH_CHILD_VARGS(n, LED_STRIP_REMAP_INDICATOR_INDEXES, n)                       \
                                                                                                   \
	static const struct led_strip_remap_indicator led_strip_remap_indicators_##n[] = {         \
		DT_INST_FOREACH_CHILD_VARGS(n, LED_STRIP_REMAP_INDICATOR, n)                       \
	};                                                                                         \
                                                                                                   \
	static const struct led_strip_remap_config led_strip_remap_config_##n = {                  \
		.chain_length = DT_INST_PROP(n, chain_length),                                     \
		.led_strip = DEVICE_DT_GET(DT_INST_PHANDLE(n, led_strip)),                         \
		.led_strip_len = DT_INST_PROP_BY_PHANDLE(n, led_strip, chain_length),              \
		.map = led_strip_remap_map_##n,                                                    \
		.map_len = DT_INST_PROP_LEN(n, map),                                               \
		.indicators = led_strip_remap_indicators_##n,                                      \
		.indicator_cnt = ARRAY_SIZE(led_strip_remap_indicators_##n),                       \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, led_strip_remap_init, NULL, &led_strip_remap_data_##n,            \
			      &led_strip_remap_config_##n, POST_KERNEL,                            \
			      CONFIG_LED_STRIP_INIT_PRIORITY, &led_strip_remap_api);

DT_INST_FOREACH_STATUS_OKAY(LED_STRIP_REMAP_INIT)
