/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#include "handler.h"

#include <zmk/keymap.h>

static uint8_t current_layer = 0;

bool handle_fn_state(const uart_comm_MessageK2D *k2d)
{
	const uart_comm_FnState *report = &k2d->payload.fn_state;

	if (report->pressed) {
		current_layer = zmk_keymap_highest_layer_active();
		zmk_keymap_layer_to(1);
	} else {
		zmk_keymap_layer_to(current_layer);
	}

	return true;
}
