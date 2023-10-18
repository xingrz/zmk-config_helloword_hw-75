/*
 * Copyright (c) 2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#include "knob_indicator.h"

#include <math.h>
#include <arm_math.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <knob/drivers/knob.h>
#include <knob/drivers/motor.h>

#include <zmk/activity.h>
#include <zmk/event_manager.h>
#include <zmk/events/activity_state_changed.h>

#define KNOB_NODE DT_ALIAS(knob)
#define MOTOR_NODE DT_PHANDLE(KNOB_NODE, motor)

#define DISPLAY_NODE DT_CHOSEN(zephyr_display)

#define SCREEN_W DT_PROP(DISPLAY_NODE, width)
#define SCREEN_H DT_PROP(DISPLAY_NODE, height)

#define MARGIN_V (10)
#define INDICATOR_BD (1)
#define INDICATOR_W (2 + INDICATOR_BD)
#define INDICATOR_H (2 + INDICATOR_BD * 2)
#define INDICATOR_T (10)

#define TRACK_H (SCREEN_H - MARGIN_V * 2)

#define PI_2 (PI / 2.0f)

#define MINIMUM_MOVEMENT (0.2f)

static const struct device *knob = DEVICE_DT_GET(KNOB_NODE);
static const struct device *motor = DEVICE_DT_GET(MOTOR_NODE);

static lv_obj_t *indicator;
static struct motor_state state;

static bool visible = false;

static float position_zero = 0.0f;

static lv_coord_t last_item_h = 0;
static lv_coord_t last_item_y = 0;

static void hide_work_cb(struct k_work *work)
{
	ARG_UNUSED(work);

	lv_obj_add_flag(indicator, LV_OBJ_FLAG_HIDDEN);
	visible = false;

	motor_inspect(motor, &state);
	position_zero = state.current_angle;
}

K_WORK_DELAYABLE_DEFINE(hide_work, hide_work_cb);

static void refresh_work_cb(struct k_work *work)
{
	ARG_UNUSED(work);

	motor_inspect(motor, &state);
	float dp = state.current_angle - position_zero;

	if (!visible) {
		if (LV_ABS(dp) >= MINIMUM_MOVEMENT) {
			visible = true;
			lv_obj_clear_flag(indicator, LV_OBJ_FLAG_HIDDEN);
		} else {
			return;
		}
	}

	enum knob_mode mode = knob_get_mode(knob);
	if (mode == KNOB_SPRING || mode == KNOB_DAMPED || mode == KNOB_SWITCH) {
		/*
		 * Always use absolute position zero for these modes
		 * TODO: Need a better way to determine
		 */
		dp = state.current_angle - PI;
	}

	if (visible) {
		float item_h = INDICATOR_H;
		float item_y = (float)TRACK_H / 2.0f;

		if (dp < -PI_2) {
			dp = -PI_2;
			position_zero = state.current_angle - dp;
		} else if (dp > PI_2) {
			dp = PI_2;
			position_zero = state.current_angle - dp;
		}

		item_y += dp / PI_2 * (float)TRACK_H / 2.0f;

		float tail_h = INDICATOR_T * state.target_voltage;
		tail_h = LV_ABS(tail_h);
		if (tail_h > 4.0f) {
			item_h += tail_h;
			item_y -= tail_h / 2.0f;
		}

		lv_coord_t fin_item_y = (lv_coord_t)(MARGIN_V + item_y - INDICATOR_H / 2.0f);
		lv_coord_t fin_item_h = (lv_coord_t)item_h;

		if (fin_item_y == last_item_y && fin_item_h == last_item_h) {
			return;
		}

		lv_obj_set_y(indicator, fin_item_y);
		lv_obj_set_height(indicator, fin_item_h);

		if (lv_obj_has_flag(indicator, LV_OBJ_FLAG_HIDDEN)) {
			lv_obj_clear_flag(indicator, LV_OBJ_FLAG_HIDDEN);
		}

		k_work_reschedule(&hide_work, K_MSEC(1000));

		last_item_y = fin_item_y;
		last_item_h = fin_item_h;
	}
}

K_WORK_DEFINE(refresh_work, refresh_work_cb);

void refresh_timer_cb()
{
	k_work_submit_to_queue(&k_sys_work_q, &refresh_work);
}

K_TIMER_DEFINE(refresh_timer, refresh_timer_cb, NULL);

static inline void knob_indicator_refresh_start(void)
{
	k_timer_start(&refresh_timer, K_MSEC(0), K_MSEC(20));
}

static inline void knob_indicator_refresh_stop(void)
{
	k_timer_stop(&refresh_timer);
}

int knob_indicator_init(lv_obj_t *parent)
{
	indicator = lv_obj_create(parent);
	lv_obj_set_size(indicator, INDICATOR_W, INDICATOR_H);
	lv_obj_set_pos(indicator, 0, MARGIN_V);
	lv_obj_set_style_bg_opa(indicator, LV_OPA_100, LV_PART_MAIN);
	lv_obj_set_style_bg_color(indicator, lv_color_black(), LV_PART_MAIN);
	lv_obj_set_style_border_opa(indicator, LV_OPA_100, LV_PART_MAIN);
	lv_obj_set_style_border_color(indicator, lv_color_white(), LV_PART_MAIN);
	lv_obj_set_style_border_side(
		indicator, LV_BORDER_SIDE_TOP | LV_BORDER_SIDE_RIGHT | LV_BORDER_SIDE_BOTTOM,
		LV_PART_MAIN);
	lv_obj_set_style_border_width(indicator, INDICATOR_BD, LV_PART_MAIN);
	lv_obj_add_flag(indicator, LV_OBJ_FLAG_HIDDEN);

	visible = false;

	motor_inspect(motor, &state);
	position_zero = state.current_angle;

	knob_indicator_refresh_start();

	return 0;
}
