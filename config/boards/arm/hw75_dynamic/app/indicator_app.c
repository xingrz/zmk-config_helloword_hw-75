/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#include <kernel.h>
#include <device.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <drivers/led_strip_remap.h>

#include "indicator_app.h"

#define STRIP_LABEL DT_LABEL(DT_CHOSEN(zmk_underglow))

static const struct device *led_strip;

static struct led_rgb color_red = { .r = 0x22, .g = 0x00, .b = 0x00 };
static struct led_rgb color_green = { .r = 0x00, .g = 0x22, .b = 0x00 };

static uint32_t state = 0;

static int indicator_update()
{
	if (!led_strip) {
		LOG_ERR("LED strip device %s not found", STRIP_LABEL);
		return -ENODEV;
	}

	return led_strip_remap_set(led_strip, "STATUS", state ? &color_red : &color_green);
}

static int indicator_app_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	led_strip = device_get_binding(STRIP_LABEL);
	return indicator_update();
}

void indicator_set_bits(uint32_t bits)
{
	state |= bits;
	indicator_update();
}

void indicator_clear_bits(uint32_t bits)
{
	state &= ~bits;
	indicator_update();
}

SYS_INIT(indicator_app_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
