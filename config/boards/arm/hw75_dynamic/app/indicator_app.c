/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#include <kernel.h>
#include <device.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <drivers/led_strip_remap.h>

#include <zmk/event_manager.h>
#include <zmk/events/activity_state_changed.h>
#include <app/events/knob_state_changed.h>

#include "indicator_app.h"

#define STRIP_LABEL DT_LABEL(DT_CHOSEN(zmk_underglow))

#define BRI_ACTIVE (5)
#define BRI_INACTIVE (1)

#define RGB(R, G, B) .r = (R), .g = (G), .b = (B)

static const struct device *led_strip;

static const struct led_rgb color_red = { RGB(0xFF, 0x00, 0x00) };
static const struct led_rgb color_green = { RGB(0x00, 0xFF, 0x00) };

static struct k_mutex lock;

static struct led_rgb current;
static bool active = true;

static uint32_t state = 0;

static inline void apply_color(struct led_rgb *c_out, const struct led_rgb *c_in)
{
	c_out->r = c_in->r;
	c_out->g = c_in->g;
	c_out->b = c_in->b;
}

static inline void apply_brightness(struct led_rgb *c_out, const struct led_rgb *c_in, uint8_t bri)
{
	c_out->r = (uint8_t)((int)c_in->r * bri / 255);
	c_out->g = (uint8_t)((int)c_in->g * bri / 255);
	c_out->b = (uint8_t)((int)c_in->b * bri / 255);
}

static void indicator_update(struct k_work *work)
{
	if (!led_strip) {
		LOG_ERR("LED strip device %s not found", STRIP_LABEL);
		return;
	}

	apply_color(&current, state ? &color_red : &color_green);

	uint8_t bri = active ? BRI_ACTIVE : BRI_INACTIVE;

	struct led_rgb color;
	apply_color(&color, &current);
	apply_brightness(&color, &current, bri);

	LOG_DBG("Update indicator, color: %02X%02X%02X, brightness: %d -> %02X%02X%02X", current.r,
		current.g, current.b, bri, color.r, color.g, color.b);

	led_strip_remap_set(led_strip, "STATUS", &color);
}

K_WORK_DEFINE(indicator_update_work, indicator_update);

static int indicator_app_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	led_strip = device_get_binding(STRIP_LABEL);
	k_mutex_init(&lock);
	k_work_submit_to_queue(&k_sys_work_q, &indicator_update_work);
	return 0;
}

void indicator_set_bits(uint32_t bits)
{
	k_mutex_lock(&lock, K_FOREVER);
	state |= bits;
	k_mutex_unlock(&lock);
	k_work_submit_to_queue(&k_sys_work_q, &indicator_update_work);
}

void indicator_clear_bits(uint32_t bits)
{
	k_mutex_lock(&lock, K_FOREVER);
	state &= ~bits;
	k_mutex_unlock(&lock);
	k_work_submit_to_queue(&k_sys_work_q, &indicator_update_work);
}

static int indicator_app_event_listener(const zmk_event_t *eh)
{
	struct zmk_activity_state_changed *activity_ev;
	struct app_knob_state_changed *knob_ev;

	if ((activity_ev = as_zmk_activity_state_changed(eh)) != NULL) {
		active = activity_ev->state == ZMK_ACTIVITY_ACTIVE;
		k_work_submit_to_queue(&k_sys_work_q, &indicator_update_work);
		return 0;
	} else if ((knob_ev = as_app_knob_state_changed(eh)) != NULL) {
		if (knob_ev->calibration == KNOB_CALIBRATE_OK) {
			indicator_clear_bits(INDICATOR_KNOB_CALIBRATING);
		} else {
			indicator_set_bits(INDICATOR_KNOB_CALIBRATING);
		}
		return 0;
	}

	return -ENOTSUP;
}

ZMK_LISTENER(indicator_app, indicator_app_event_listener);
ZMK_SUBSCRIPTION(indicator_app, zmk_activity_state_changed);
ZMK_SUBSCRIPTION(indicator_app, app_knob_state_changed);

SYS_INIT(indicator_app_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
