/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#include <kernel.h>
#include <device.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <drivers/led_strip_remap.h>

#include <zmk/usb.h>
#include <zmk/event_manager.h>
#include <zmk/events/usb_conn_state_changed.h>

#define STRIP_LABEL DT_LABEL(DT_CHOSEN(zmk_underglow))

static const struct device *led_strip;

static struct led_rgb color_idle = { .r = 0x22, .g = 0x00, .b = 0x00 };
static struct led_rgb color_connected = { .r = 0x00, .g = 0x22, .b = 0x00 };

static int led_indicator_usb_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	led_strip = device_get_binding(STRIP_LABEL);
	if (!led_strip) {
		LOG_ERR("LED strip device %s not found", STRIP_LABEL);
		return -ENODEV;
	}

	return led_strip_remap_clear(led_strip, "STATUS");
}

static int led_indicator_usb_event_listener(const zmk_event_t *eh)
{
	if (!led_strip) {
		return -ENODEV;
	}

	if (as_zmk_usb_conn_state_changed(eh)) {
		led_strip_remap_set(led_strip, "STATUS",
				    zmk_usb_is_hid_ready() ? &color_connected : &color_idle);
		return 0;
	}

	return -ENOTSUP;
}

ZMK_LISTENER(led_indicator_usb, led_indicator_usb_event_listener);
ZMK_SUBSCRIPTION(led_indicator_usb, zmk_usb_conn_state_changed);

SYS_INIT(led_indicator_usb_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
