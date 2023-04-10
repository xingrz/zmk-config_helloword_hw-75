/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <knob/drivers/knob.h>
#include <knob/drivers/motor.h>

#include <zmk/activity.h>
#include <zmk/event_manager.h>
#include <zmk/events/activity_state_changed.h>
#include <app/events/knob_state_changed.h>

#include "knob_app.h"

#define KNOB_NODE DT_ALIAS(knob)
#define MOTOR_NODE DT_PHANDLE(KNOB_NODE, motor)

#define KNOB_APP_THREAD_STACK_SIZE 1024
#define KNOB_APP_THREAD_PRIORITY 10

static const struct device *knob = DEVICE_DT_GET(KNOB_NODE);
static const struct device *motor = DEVICE_DT_GET(MOTOR_NODE);

static bool motor_demo = false;

static struct k_work_delayable knob_enable_report_work;

static void knob_app_enable_report_delayed_work(struct k_work *work)
{
	ARG_UNUSED(work);
	knob_set_encoder_report(knob, true);
}

static void knob_app_enable_report_delayed(void)
{
	k_work_reschedule(&knob_enable_report_work, K_MSEC(500));
}

static void knob_app_disable_report(void)
{
	struct k_work_sync sync;
	k_work_cancel_delayable_sync(&knob_enable_report_work, &sync);
	knob_set_encoder_report(knob, false);
}

K_THREAD_STACK_DEFINE(knob_work_stack_area, KNOB_APP_THREAD_STACK_SIZE);
static struct k_work_q knob_work_q;

ZMK_EVENT_IMPL(app_knob_state_changed);

static void knob_app_calibrate(struct k_work *work)
{
	ZMK_EVENT_RAISE(new_app_knob_state_changed((struct app_knob_state_changed){
		.enable = false,
		.demo = false,
		.calibration = KNOB_CALIBRATING,
	}));

	int ret = motor_calibrate_auto(motor);
	if (ret == 0) {
		knob_set_mode(knob, KNOB_ENCODER);
		knob_app_enable_report_delayed();

		ZMK_EVENT_RAISE(new_app_knob_state_changed((struct app_knob_state_changed){
			.enable = true,
			.demo = false,
			.calibration = KNOB_CALIBRATE_OK,
		}));
	} else {
		LOG_ERR("Motor is not calibrated");

		ZMK_EVENT_RAISE(new_app_knob_state_changed((struct app_knob_state_changed){
			.enable = false,
			.demo = false,
			.calibration = KNOB_CALIBRATE_FAILED,
		}));
	}
}

K_WORK_DEFINE(calibrate_work, knob_app_calibrate);

bool knob_app_get_demo(void)
{
	return motor_demo;
}

void knob_app_set_demo(bool demo)
{
	if (!knob || !motor) {
		return;
	}

	if (!motor_is_calibrated(motor)) {
		return;
	}

	motor_demo = demo;
	if (demo) {
		knob_app_disable_report();
	} else {
		knob_set_mode(knob, KNOB_ENCODER);
		knob_app_enable_report_delayed();
	}

	ZMK_EVENT_RAISE(new_app_knob_state_changed((struct app_knob_state_changed){
		.enable = true,
		.demo = demo,
		.calibration = KNOB_CALIBRATE_OK,
	}));
}

static int knob_app_event_listener(const zmk_event_t *eh)
{
	if (!knob || !motor) {
		return -ENODEV;
	}

	if (!motor_is_calibrated(motor) || motor_demo) {
		return 0;
	}

	if (as_zmk_activity_state_changed(eh)) {
		bool active = zmk_activity_get_state() == ZMK_ACTIVITY_ACTIVE;

		knob_set_enable(knob, active);

		ZMK_EVENT_RAISE(new_app_knob_state_changed((struct app_knob_state_changed){
			.enable = active,
			.demo = false,
			.calibration = KNOB_CALIBRATE_OK,
		}));

		return 0;
	}

	return -ENOTSUP;
}

static int knob_app_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	k_work_init_delayable(&knob_enable_report_work, knob_app_enable_report_delayed_work);

	k_work_queue_start(&knob_work_q, knob_work_stack_area,
			   K_THREAD_STACK_SIZEOF(knob_work_stack_area), KNOB_APP_THREAD_PRIORITY,
			   NULL);

	k_work_submit_to_queue(&knob_work_q, &calibrate_work);

	return 0;
}

ZMK_LISTENER(knob_app, knob_app_event_listener);
ZMK_SUBSCRIPTION(knob_app, zmk_activity_state_changed);

SYS_INIT(knob_app_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
