/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include <string.h>

#include <zmk/keymap.h>
#include <zmk/event_manager.h>
#include <zmk/events/layer_state_changed.h>

#include "report.h"

static bool fn_pressed = false;

static void report_fn_state_changed(bool pressed)
{
	uart_comm_MessageK2D k2d = uart_comm_MessageK2D_init_zero;
	k2d.action = uart_comm_Action_FN_STATE_CHANGED;
	k2d.which_payload = uart_comm_MessageK2D_fn_state_tag;
	k2d.payload.fn_state.pressed = pressed;

	uart_comm_report(&k2d);
}

static int report_fn_state_event_listener(const zmk_event_t *eh)
{
	if (as_zmk_layer_state_changed(eh)) {
		uint8_t index = zmk_keymap_highest_layer_active();
		bool current = strncmp(zmk_keymap_layer_label(index), "FN", 2) == 0;
		if (!fn_pressed && current) {
			report_fn_state_changed(true);
		} else if (fn_pressed && !current) {
			report_fn_state_changed(false);
		}
		fn_pressed = current;
		return 0;
	}

	return -ENOTSUP;
}

ZMK_LISTENER(report_fn_state, report_fn_state_event_listener);
ZMK_SUBSCRIPTION(report_fn_state, zmk_layer_state_changed);
