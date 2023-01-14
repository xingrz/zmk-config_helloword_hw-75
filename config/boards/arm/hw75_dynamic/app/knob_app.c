/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#include <kernel.h>
#include <device.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <knob/drivers/knob.h>

#include <zmk/activity.h>
#include <zmk/usb.h>
#include <zmk/event_manager.h>
#include <zmk/events/activity_state_changed.h>
#include <zmk/events/usb_conn_state_changed.h>

static const struct device *knob;

static int knob_app_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	knob = device_get_binding("KNOB");
	if (knob) {
		LOG_INF("Found Knob device");
	} else {
		LOG_ERR("Knob device not found");
		return -EINVAL;
	}

	knob_set_mode(knob, KNOB_ENCODER);
	knob_set_encoder_report(knob, true);

	return 0;
}

static int knob_app_event_listener(const zmk_event_t *eh)
{
	bool active, hid_ready;

	if (!knob) {
		return -ENODEV;
	}

	if (as_zmk_activity_state_changed(eh) || as_zmk_usb_conn_state_changed(eh)) {
		active = zmk_activity_get_state() == ZMK_ACTIVITY_ACTIVE;
		hid_ready = zmk_usb_is_hid_ready();
		knob_set_enable(knob, active && hid_ready);
		return 0;
	}

	return -ENOTSUP;
}

ZMK_LISTENER(knob_app, knob_app_event_listener);
ZMK_SUBSCRIPTION(knob_app, zmk_activity_state_changed);
ZMK_SUBSCRIPTION(knob_app, zmk_usb_conn_state_changed);

SYS_INIT(knob_app_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
