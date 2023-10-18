/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/settings/settings.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <knob/drivers/knob.h>
#include <knob/drivers/motor.h>

#include <zmk/activity.h>
#include <zmk/keymap.h>
#include <zmk/event_manager.h>
#include <zmk/events/activity_state_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <app/events/knob_state_changed.h>

#include "knob_app.h"

#define KNOB_NODE DT_ALIAS(knob)
#define MOTOR_NODE DT_PHANDLE(KNOB_NODE, motor)

#define KNOB_APP_THREAD_STACK_SIZE 1024
#define KNOB_APP_THREAD_PRIORITY 10

#define KEYMAP_NODE DT_INST(0, zmk_keymap)
#define KEYMAP_LAYER_CHILD_LEN(node) 1 +
#define KEYMAP_LAYERS_NUM (DT_FOREACH_CHILD(KEYMAP_NODE, KEYMAP_LAYER_CHILD_LEN) 0)

#define LAYER_LABEL(node)                                                                          \
	COND_CODE_0(DT_NODE_HAS_PROP(node, label), (NULL), (DT_PROP(node, label))),

#define LAYER_PROFILE(node) DT_PROP_OR(node, profile, DT_NODELABEL(profile_encoder))

#define LAYER_PREF(node)                                                                           \
	{                                                                                          \
		.active = false,                                                                   \
		.name = DT_NODE_FULL_NAME(node),                                                   \
		.mode = DT_REG_ADDR(LAYER_PROFILE(node)),                                          \
		.ppr = DT_PROP(node, ppr),                                                         \
		.torque_limit = (float)DT_PROP(LAYER_PROFILE(node), torque_limit_mv) / 1000.0f,    \
	},

static const char *layer_names[KEYMAP_LAYERS_NUM] = { DT_FOREACH_CHILD(KEYMAP_NODE, LAYER_LABEL) };

static const struct knob_pref layer_prefs[KEYMAP_LAYERS_NUM] = { DT_FOREACH_CHILD(KEYMAP_NODE,
										  LAYER_PREF) };

static const struct device *knob = DEVICE_DT_GET(KNOB_NODE);
static const struct device *motor = DEVICE_DT_GET(MOTOR_NODE);

static bool motor_demo = false;

static struct knob_pref knob_prefs[KEYMAP_LAYERS_NUM];

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

static void knob_app_apply_pref(uint8_t layer_id);

static void knob_app_calibrate(struct k_work *work)
{
	ZMK_EVENT_RAISE(new_app_knob_state_changed((struct app_knob_state_changed){
		.enable = false,
		.demo = false,
		.calibration = KNOB_CALIBRATING,
	}));

	int ret = motor_calibrate_auto(motor);
	if (ret == 0) {
		knob_app_apply_pref(zmk_keymap_highest_layer_active());

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

#ifdef CONFIG_SETTINGS
static int knob_app_settings_load_cb(const char *name, size_t len, settings_read_cb read_cb,
				     void *cb_arg, void *param)
{
	const char *next;
	struct knob_pref loader[KEYMAP_LAYERS_NUM];
	int ret;

	if (settings_name_steq(name, "prefs", &next) && !next) {
		if (len != sizeof(loader)) {
			LOG_ERR("Invalid knob prefs size: %d", len);
			return -EINVAL;
		}

		ret = read_cb(cb_arg, &loader, sizeof(loader));
		if (ret < 0) {
			LOG_ERR("Failed to read knob prefs: %d", ret);
			return 0;
		}

		for (uint8_t i = 0; i < ARRAY_SIZE(loader); i++) {
			LOG_DBG("Read knob pref %d: layer=\"%s\", active=%d", i, loader[i].name,
				loader[i].active);

			if (!loader[i].active) {
				continue;
			}

			for (uint8_t j = 0; j < ARRAY_SIZE(knob_prefs); j++) {
				if (strcmp(loader[i].name, knob_prefs[j].name) == 0) {
					memcpy(&knob_prefs[j], &loader[i],
					       sizeof(struct knob_pref));

					LOG_DBG("Loaded knob pref %d for layer %d \"%s\": mode=%d, ppr=%d, torque_limit=%.03f",
						i, j, knob_prefs[j].name, knob_prefs[j].mode,
						knob_prefs[j].ppr, knob_prefs[j].torque_limit);

					break;
				}
			}
		}

		LOG_DBG("Loaded knob prefs");

		return ret;
	}

	return -ENOENT;
}

static void knob_app_save_prefs_work(struct k_work *work)
{
	ARG_UNUSED(work);
	int ret = settings_save_one("app/knob/prefs", &knob_prefs, sizeof(knob_prefs));
	if (ret != 0) {
		LOG_ERR("Failed saving prefs: %d", ret);
	} else {
		LOG_DBG("Saved knob prefs");
	}
}

static struct k_work_delayable knob_app_save_work;
#endif

int knob_app_save_prefs(void)
{
#ifdef CONFIG_SETTINGS
	int ret = k_work_reschedule(&knob_app_save_work, K_MSEC(CONFIG_ZMK_SETTINGS_SAVE_DEBOUNCE));
	return MIN(ret, 0);
#else
	return 0;
#endif
}

int knob_app_get_prefs(const struct knob_pref **prefs, const char ***names)
{
	*prefs = &knob_prefs[0];
	if (names != NULL) {
		*names = &layer_names[0];
	}
	return KEYMAP_LAYERS_NUM;
}

const struct knob_pref *knob_app_get_pref(uint8_t layer_id)
{
	if (layer_id >= KEYMAP_LAYERS_NUM) {
		return NULL;
	}
	return &knob_prefs[layer_id];
}

static void knob_app_apply_pref(uint8_t layer_id)
{
	knob_app_disable_report();

	struct knob_pref *pref = &knob_prefs[layer_id];
	if (knob_get_mode(knob) != pref->mode) {
		knob_set_mode(knob, pref->mode);
	}
	knob_set_encoder_ppr(knob, pref->ppr);
	motor_set_torque_limit(motor, pref->torque_limit);

	knob_app_enable_report_delayed();

	LOG_DBG("Applied knob prefs for layer %d, pref active: %d", layer_id, pref->active);
}

void knob_app_set_pref(uint8_t layer_id, struct knob_pref *pref)
{
	if (layer_id >= KEYMAP_LAYERS_NUM) {
		return;
	}

	memcpy(&knob_prefs[layer_id], pref, sizeof(struct knob_pref));
	knob_app_save_prefs();

	if (layer_id == zmk_keymap_highest_layer_active()) {
		knob_app_apply_pref(layer_id);
	}
}

void knob_app_reset_pref(uint8_t layer_id)
{
	if (layer_id >= KEYMAP_LAYERS_NUM) {
		return;
	}

	memcpy(&knob_prefs[layer_id], &layer_prefs[layer_id], sizeof(struct knob_pref));
	knob_app_save_prefs();

	if (layer_id == zmk_keymap_highest_layer_active()) {
		knob_app_apply_pref(layer_id);
	}
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
	} else if (as_zmk_layer_state_changed(eh)) {
		knob_app_apply_pref(zmk_keymap_highest_layer_active());
		return 0;
	}

	return -ENOTSUP;
}

static int knob_app_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	int ret;

	memcpy(&knob_prefs, &layer_prefs, sizeof(layer_prefs));

#ifdef CONFIG_SETTINGS
	ret = settings_subsys_init();
	if (ret) {
		LOG_ERR("Failed to initializing settings subsys: %d", ret);
	}

	ret = settings_load_subtree_direct("app/knob", knob_app_settings_load_cb, NULL);
	if (ret) {
		LOG_ERR("Failed to load knob settings: %d", ret);
	}

	k_work_init_delayable(&knob_app_save_work, knob_app_save_prefs_work);
#endif

	k_work_init_delayable(&knob_enable_report_work, knob_app_enable_report_delayed_work);

	k_work_queue_start(&knob_work_q, knob_work_stack_area,
			   K_THREAD_STACK_SIZEOF(knob_work_stack_area), KNOB_APP_THREAD_PRIORITY,
			   NULL);

	k_work_submit_to_queue(&knob_work_q, &calibrate_work);

	return 0;
}

ZMK_LISTENER(knob_app, knob_app_event_listener);
ZMK_SUBSCRIPTION(knob_app, zmk_activity_state_changed);
ZMK_SUBSCRIPTION(knob_app, zmk_layer_state_changed);

SYS_INIT(knob_app_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
