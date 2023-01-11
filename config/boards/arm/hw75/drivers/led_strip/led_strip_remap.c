/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_led_strip_remap

#include <drivers/led_strip.h>

#include <zephyr.h>
#include <device.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct led_strip_remap_data {
	struct led_rgb *pixels;
};

struct led_strip_remap_config {
	uint32_t chain_length;
	const struct device *led_strip;
	uint32_t led_strip_len;
	const uint32_t *map;
	uint32_t map_len;
};

static int led_strip_remap_update_rgb(const struct device *dev, struct led_rgb *pixels,
				      size_t num_pixels)
{
	struct led_strip_remap_data *data = dev->data;
	const struct led_strip_remap_config *config = dev->config;

	if (num_pixels > config->map_len) {
		num_pixels = config->map_len;
	}

	for (uint32_t i = 0; i < num_pixels; i++) {
		memcpy(&data->pixels[config->map[i]], &pixels[i], sizeof(struct led_rgb));
	}

	return led_strip_update_rgb(config->led_strip, data->pixels, num_pixels);
}

static int led_strip_remap_update_channels(const struct device *dev, uint8_t *channels,
					   size_t num_channels)
{
	LOG_ERR("update_channels not implemented");
	return -ENOTSUP;
}

static int led_strip_remap_init(const struct device *dev)
{
	const struct led_strip_remap_config *config = dev->config;

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

	return 0;
}

static const struct led_strip_driver_api led_strip_remap_api = {
	.update_rgb = led_strip_remap_update_rgb,
	.update_channels = led_strip_remap_update_channels,
};

#define LED_STRIP_REMAP_INIT(n)                                                                    \
	static struct led_rgb led_strip_remap_pixels_##n[DT_INST_PROP_LEN(n, map)] = { 0 };        \
                                                                                                   \
	static struct led_strip_remap_data led_strip_remap_data_##n = {                            \
		.pixels = led_strip_remap_pixels_##n,                                              \
	};                                                                                         \
                                                                                                   \
	static const uint32_t led_strip_remap_map_##n[] = DT_INST_PROP(n, map);                    \
                                                                                                   \
	static const struct led_strip_remap_config led_strip_remap_config_##n = {                  \
		.chain_length = DT_INST_PROP(n, chain_length),                                     \
		.led_strip = DEVICE_DT_GET(DT_INST_PHANDLE(n, led_strip)),                         \
		.led_strip_len = DT_INST_PROP_BY_PHANDLE(n, led_strip, chain_length),              \
		.map = led_strip_remap_map_##n,                                                    \
		.map_len = DT_INST_PROP_LEN(n, map),                                               \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, led_strip_remap_init, NULL, &led_strip_remap_data_##n,            \
			      &led_strip_remap_config_##n, POST_KERNEL,                            \
			      CONFIG_LED_STRIP_INIT_PRIORITY, &led_strip_remap_api);

DT_INST_FOREACH_STATUS_OKAY(LED_STRIP_REMAP_INIT)
