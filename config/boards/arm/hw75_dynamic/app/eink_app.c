/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zephyr/drivers/display.h>

#include <zmk/event_manager.h>
#include <app/events/eink_state_changed.h>

#include "eink_app.h"

#define EINK_NODE DT_ALIAS(eink)
#define EINK_WIDTH DT_PROP(EINK_NODE, width)
#define EINK_HEIGHT DT_PROP(EINK_NODE, height)

static const struct device *eink = DEVICE_DT_GET(EINK_NODE);

ZMK_EVENT_IMPL(app_eink_state_changed);

int eink_update(const uint8_t *image, uint32_t image_len, bool partial)
{
	return eink_update_region(image, image_len, 0, 0, EINK_WIDTH, EINK_HEIGHT, partial);
}

int eink_update_region(const uint8_t *image, uint32_t image_len, uint32_t x, uint32_t y,
		       uint32_t width, uint32_t height, bool partial)
{
	if (x >= EINK_WIDTH || y >= EINK_HEIGHT || x + width > EINK_WIDTH ||
	    y + height > EINK_HEIGHT) {
		LOG_ERR("Invalid partial update region: (%d, %d, %d, %d)", x, y, width, height);
		return -EINVAL;
	}

	if (image_len != width * height / 8) {
		LOG_ERR("Invalid image length: %d", image_len);
		return -EINVAL;
	}

	struct display_buffer_descriptor desc = {
		.buf_size = image_len,
		.width = width,
		.height = height,
		.pitch = width,
	};

	LOG_DBG("Start updating E-Ink");

	ZMK_EVENT_RAISE(new_app_eink_state_changed((struct app_eink_state_changed){
		.busy = true,
	}));

	if (!partial) {
		display_blanking_on(eink);
	}

	int ret = display_write(eink, x, y, &desc, image);
	if (ret != 0) {
		LOG_ERR("Failed updating E-ink image: %d", ret);
		return ret;
	}

	if (!partial) {
		display_blanking_off(eink);
	}

	ZMK_EVENT_RAISE(new_app_eink_state_changed((struct app_eink_state_changed){
		.busy = false,
	}));

	LOG_DBG("E-Ink update finished");

	return 0;
}
