/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#include "knob_status.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <app/events/knob_state_changed.h>

#include "icons.h"
#include "strings.h"

#define DISPLAY_NODE DT_CHOSEN(zephyr_display)

#define SCREEN_W DT_PROP(DISPLAY_NODE, width)
#define SCREEN_H DT_PROP(DISPLAY_NODE, height)
#define ITEM_SIZE MIN(SCREEN_W, SCREEN_H)

LV_FONT_DECLARE(mono_19);
LV_FONT_DECLARE(zfull_9);

static lv_obj_t *cont;
static lv_obj_t *icon;
static lv_obj_t *label;

static void knob_status_update_cb(enum knob_calibration_state state)
{
	if (state == KNOB_CALIBRATE_OK) {
		lv_obj_add_flag(cont, LV_OBJ_FLAG_HIDDEN);
	} else if (state == KNOB_CALIBRATE_FAILED) {
		lv_label_set_text(icon, ICON_ERROR);
		lv_label_set_text(label, STRING_KNOB_CALIB_FAIL);
	} else if (state == KNOB_CALIBRATING) {
		lv_label_set_text(icon, ICON_CALI);
		lv_label_set_text(label, STRING_KNOB_CALIB);
	}
}

static enum knob_calibration_state knob_status_get_state(const zmk_event_t *eh)
{
	struct app_knob_state_changed *knob_ev = as_app_knob_state_changed(eh);
	return knob_ev->calibration;
}

ZMK_DISPLAY_WIDGET_LISTENER(knob_status_subscribtion, enum knob_calibration_state,
			    knob_status_update_cb, knob_status_get_state)

ZMK_SUBSCRIPTION(knob_status_subscribtion, app_knob_state_changed);

int knob_status_init(lv_obj_t *parent)
{
	cont = lv_obj_create(parent);
	lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
	lv_obj_set_style_bg_opa(cont, LV_OPA_100, LV_PART_MAIN);
	lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
			      LV_FLEX_ALIGN_CENTER);

	lv_obj_t *head = lv_obj_create(cont);
	lv_obj_set_size(head, ITEM_SIZE, ITEM_SIZE);

	icon = lv_label_create(head);
	lv_obj_set_style_text_font(icon, &mono_19, LV_PART_MAIN);
	lv_obj_center(icon);

	label = lv_label_create(cont);
	lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
	lv_obj_set_size(label, 9, LV_SIZE_CONTENT);
	lv_obj_set_style_text_font(label, &zfull_9, LV_PART_MAIN);
	lv_obj_set_style_pad_ver(label, 4, LV_PART_MAIN);

	knob_status_subscribtion_init();

	return 0;
}
