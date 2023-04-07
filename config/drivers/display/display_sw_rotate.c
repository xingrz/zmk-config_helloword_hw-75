/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_display_sw_rotate

#include <zephyr/drivers/display.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define BITS_PER_ITEM 8

struct sw_rotate_data {
	uint8_t *buffer;
};

struct sw_rotate_config {
	const struct device *dst;
	uint16_t dst_width;
	uint16_t dst_height;
	uint16_t src_width;
	uint16_t src_height;
};

static int sw_rotate_blanking_on(const struct device *dev)
{
	const struct sw_rotate_config *config = dev->config;

	return display_blanking_on(config->dst);
}

static int sw_rotate_blanking_off(const struct device *dev)
{
	const struct sw_rotate_config *config = dev->config;

	return display_blanking_off(config->dst);
}

static int sw_rotate_write(const struct device *dev, const uint16_t x, const uint16_t y,
			   const struct display_buffer_descriptor *desc, const void *buf)
{
	struct sw_rotate_data *data = dev->data;
	const struct sw_rotate_config *config = dev->config;

	if (desc->pitch != desc->width) {
		LOG_ERR("Unsupported mode");
		return -1;
	}

	const struct display_buffer_descriptor desc_rot = {
		.buf_size = desc->buf_size,
		.width = desc->height,
		.height = desc->width,
		.pitch = desc->height,
	};

	const uint8_t *s = (const uint8_t *)buf;
	const uint16_t sw = desc->width / BITS_PER_ITEM;
	const uint16_t sh = desc->height;

	uint8_t *d = data->buffer;
	uint16_t dx, dy;

	for (uint16_t sy = 0; sy < sh; sy++) {
		for (uint16_t sx = 0; sx < sw; sx++) {
			dx = (desc->height - 1) - sy;
			dy = sx;
			d[dx + sh * dy] = s[sw * sy + sx];
		}
	}

	const uint16_t dst_x = config->dst_width - y - desc->height;
	const uint16_t dst_y = x;
	return display_write(config->dst, dst_x, dst_y, &desc_rot, data->buffer);
}

static int sw_rotate_read(const struct device *dev, const uint16_t x, const uint16_t y,
			  const struct display_buffer_descriptor *desc, void *buf)
{
	LOG_ERR("Unsupported");
	return -ENOTSUP;
}

static void *sw_rotate_get_framebuffer(const struct device *dev)
{
	LOG_ERR("Unsupported");
	return NULL;
}

static int sw_rotate_set_brightness(const struct device *dev, const uint8_t brightness)
{
	const struct sw_rotate_config *config = dev->config;

	return display_set_brightness(config->dst, brightness);
}

static int sw_rotate_set_contrast(const struct device *dev, const uint8_t contrast)
{
	const struct sw_rotate_config *config = dev->config;

	return display_set_contrast(config->dst, contrast);
}

static void sw_rotate_get_capabilities(const struct device *dev,
				       struct display_capabilities *capabilities)
{
	const struct sw_rotate_config *config = dev->config;

	display_get_capabilities(config->dst, capabilities);
	capabilities->x_resolution = config->src_width;
	capabilities->y_resolution = config->src_height;
	if (capabilities->screen_info & SCREEN_INFO_MONO_VTILED) {
		capabilities->screen_info &= ~SCREEN_INFO_MONO_VTILED;
	} else {
		capabilities->screen_info |= SCREEN_INFO_MONO_VTILED;
	}
}

static int sw_rotate_set_pixel_format(const struct device *dev,
				      const enum display_pixel_format pixel_format)
{
	const struct sw_rotate_config *config = dev->config;

	return display_set_pixel_format(config->dst, pixel_format);
}

static int sw_rotate_set_orientation(const struct device *dev,
				     const enum display_orientation orientation)
{
	LOG_ERR("Unsupported");
	return -ENOTSUP;
}

static int sw_rotate_init(const struct device *dev)
{
	const struct sw_rotate_config *config = dev->config;

	if (config->src_width != config->dst_height) {
		LOG_ERR("%s: width (%d) should be the same with the height of device %s (%d)",
			dev->name, config->src_width, config->dst->name, config->dst_height);
		return -EINVAL;
	}

	if (config->src_height != config->dst_width) {
		LOG_ERR("%s: height (%d) should be the same with the width of device %s (%d)",
			dev->name, config->src_height, config->dst->name, config->dst_width);
		return -EINVAL;
	}

	LOG_DBG("Bond display %s (%d x %d)", config->dst->name, config->dst_width,
		config->dst_height);

	return 0;
}

static const struct display_driver_api sw_rotate_api = {
	.blanking_on = sw_rotate_blanking_on,
	.blanking_off = sw_rotate_blanking_off,
	.write = sw_rotate_write,
	.read = sw_rotate_read,
	.get_framebuffer = sw_rotate_get_framebuffer,
	.set_brightness = sw_rotate_set_brightness,
	.set_contrast = sw_rotate_set_contrast,
	.get_capabilities = sw_rotate_get_capabilities,
	.set_pixel_format = sw_rotate_set_pixel_format,
	.set_orientation = sw_rotate_set_orientation,
};

#define SW_ROTATE_BUFFER_SIZE(n)                                                                   \
	(DT_INST_PROP(n, height) * DT_INST_PROP_OR(n, buffer_lines, 32) / BITS_PER_ITEM)

#define SW_ROTATE_INIT(n)                                                                          \
	static uint8_t sw_rotate_buffer_##n[SW_ROTATE_BUFFER_SIZE(n)] = {};                        \
                                                                                                   \
	static struct sw_rotate_data sw_rotate_data_##n = {                                        \
		.buffer = sw_rotate_buffer_##n,                                                    \
	};                                                                                         \
                                                                                                   \
	static const struct sw_rotate_config sw_rotate_config_##n = {                              \
		.dst = DEVICE_DT_GET(DT_INST_PHANDLE(n, display)),                                 \
		.dst_width = DT_INST_PROP_BY_PHANDLE(n, display, width),                           \
		.dst_height = DT_INST_PROP_BY_PHANDLE(n, display, height),                         \
		.src_width = DT_INST_PROP(n, width),                                               \
		.src_height = DT_INST_PROP(n, height),                                             \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, sw_rotate_init, NULL, &sw_rotate_data_##n, &sw_rotate_config_##n, \
			      POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY, &sw_rotate_api);

DT_INST_FOREACH_STATUS_OKAY(SW_ROTATE_INIT)
