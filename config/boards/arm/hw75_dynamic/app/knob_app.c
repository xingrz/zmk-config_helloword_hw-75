/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#include <kernel.h>
#include <device.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <knob/drivers/knob.h>
#include <knob/drivers/motor.h>

#include <zmk/activity.h>
#include <zmk/event_manager.h>
#include <zmk/events/activity_state_changed.h>

static const struct device *knob;
static const struct device *motor;

static int knob_app_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	knob = device_get_binding("KNOB");
	if (!knob) {
		LOG_ERR("Knob device not found");
		return -ENODEV;
	}

	motor = device_get_binding("MOTOR");
	if (!motor) {
		LOG_ERR("Motor device not found");
		return -ENODEV;
	}

	int ret = motor_calibrate_auto(motor);
	if (ret != 0) {
		LOG_ERR("Motor is not calibrated");
		return -EIO;
	}

	knob_set_mode(knob, KNOB_ENCODER);
	knob_set_encoder_report(knob, true);

	return 0;
}

static int knob_app_event_listener(const zmk_event_t *eh)
{
	bool active;

	if (!knob) {
		return -ENODEV;
	}

	if (as_zmk_activity_state_changed(eh)) {
		if (motor_is_calibrated(motor)) {
			active = zmk_activity_get_state() == ZMK_ACTIVITY_ACTIVE;
			knob_set_enable(knob, active);
		}
		return 0;
	}

	return -ENOTSUP;
}

ZMK_LISTENER(knob_app, knob_app_event_listener);
ZMK_SUBSCRIPTION(knob_app, zmk_activity_state_changed);

SYS_INIT(knob_app_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
