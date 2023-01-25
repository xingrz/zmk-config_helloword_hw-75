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

#include "knob_app.h"
#include "indicator_app.h"

#define KNOB_APP_THREAD_STACK_SIZE 1024
#define KNOB_APP_THREAD_PRIORITY 10

static const struct device *knob;
static const struct device *motor;

static bool motor_demo = false;

K_THREAD_STACK_DEFINE(knob_work_stack_area, KNOB_APP_THREAD_STACK_SIZE);
static struct k_work_q knob_work_q;

static void knob_app_calibrate(struct k_work *work)
{
	indicator_set_bits(INDICATOR_KNOB_CALIBRATING);

	int ret = motor_calibrate_auto(motor);
	if (ret != 0) {
		LOG_ERR("Motor is not calibrated");
		return;
	}

	knob_set_mode(knob, KNOB_ENCODER);
	knob_set_encoder_report(knob, true);

	indicator_clear_bits(INDICATOR_KNOB_CALIBRATING);
}

K_WORK_DEFINE(calibrate_work, knob_app_calibrate);

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

	k_work_queue_start(&knob_work_q, knob_work_stack_area,
			   K_THREAD_STACK_SIZEOF(knob_work_stack_area), KNOB_APP_THREAD_PRIORITY,
			   NULL);

	k_work_submit_to_queue(&knob_work_q, &calibrate_work);

	return 0;
}

static void knob_app_update_motor_state(void)
{
	if (!knob || !motor) {
		return;
	}

	if (motor_is_calibrated(motor) && !motor_demo) {
		knob_set_enable(knob, zmk_activity_get_state() == ZMK_ACTIVITY_ACTIVE);
	}
}

static int knob_app_event_listener(const zmk_event_t *eh)
{
	if (!knob) {
		return -ENODEV;
	}

	if (as_zmk_activity_state_changed(eh)) {
		knob_app_update_motor_state();
		return 0;
	}

	return -ENOTSUP;
}

bool knob_app_get_demo(void)
{
	return motor_demo;
}

void knob_app_set_demo(bool demo)
{
	motor_demo = demo;
	if (demo) {
		knob_set_encoder_report(knob, false);
	} else {
		knob_set_mode(knob, KNOB_ENCODER);
		knob_set_encoder_report(knob, true);
	}
	knob_app_update_motor_state();
}

ZMK_LISTENER(knob_app, knob_app_event_listener);
ZMK_SUBSCRIPTION(knob_app, zmk_activity_state_changed);

SYS_INIT(knob_app_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
