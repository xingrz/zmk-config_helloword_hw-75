/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#include <kernel.h>
#include <device.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <drivers/display.h>
#include <drivers/display/ssd16xx.h>

#include <zmk/event_manager.h>
#include <app/events/eink_state_changed.h>

#include "eink_app.h"

#define EINK_NODE DT_NODELABEL(eink)
#define EINK_WIDTH DT_PROP(EINK_NODE, width)
#define EINK_HEIGHT DT_PROP(EINK_NODE, height)

static const struct device *eink;

static const struct device *ssd16xx;
static bool ssd16xx_inited = false;

ZMK_EVENT_IMPL(app_eink_state_changed);

int eink_update(const uint8_t *image, uint32_t image_len)
{
	if (!eink || !ssd16xx) {
		LOG_ERR("E-Ink device not found");
		return -ENODEV;
	}

	if (image_len != EINK_WIDTH * EINK_HEIGHT / 8) {
		LOG_ERR("Invalid image length: %d", image_len);
		return -EINVAL;
	}

	struct display_buffer_descriptor desc = {
		.buf_size = image_len,
		.width = EINK_WIDTH,
		.height = EINK_HEIGHT,
		.pitch = EINK_WIDTH,
	};

	LOG_DBG("Start updating E-Ink");

	ZMK_EVENT_RAISE(new_app_eink_state_changed((struct app_eink_state_changed){
		.busy = true,
	}));

	if (!ssd16xx_inited) {
		ssd16xx_clear(ssd16xx);
		ssd16xx_inited = true;
	}

	int ret = display_write(eink, 0, 0, &desc, image);
	if (ret != 0) {
		LOG_ERR("Failed updating E-ink image: %d", ret);
		return ret;
	}

	ZMK_EVENT_RAISE(new_app_eink_state_changed((struct app_eink_state_changed){
		.busy = false,
	}));

	LOG_DBG("E-Ink update finished");

	return 0;
}

static int eink_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	eink = device_get_binding("EINK");
	ssd16xx = device_get_binding("SSD16XX");
	if (!eink || !ssd16xx) {
		LOG_ERR("E-Ink device not found");
		return -ENODEV;
	}

	return 0;
}

SYS_INIT(eink_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
