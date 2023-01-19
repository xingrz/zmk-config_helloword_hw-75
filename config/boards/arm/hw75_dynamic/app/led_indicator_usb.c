/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#include <kernel.h>
#include <device.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <drivers/led_strip_remap.h>

#define STRIP_LABEL DT_LABEL(DT_CHOSEN(zmk_underglow))

static const struct device *led_strip;

static struct led_rgb color_connected = { .r = 0x00, .g = 0x22, .b = 0x00 };

static int led_indicator_usb_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	led_strip = device_get_binding(STRIP_LABEL);
	if (!led_strip) {
		LOG_ERR("LED strip device %s not found", STRIP_LABEL);
		return -ENODEV;
	}

	return led_strip_remap_set(led_strip, "STATUS", &color_connected);
}

SYS_INIT(led_indicator_usb_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
