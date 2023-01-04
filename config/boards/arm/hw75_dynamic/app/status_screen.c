/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#include <zmk/display/status_screen.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include "widgets/layer_status.h"

static struct custom_widget_layer_status layer_status_widget;

lv_obj_t *zmk_display_status_screen()
{
	lv_obj_t *screen;

	screen = lv_obj_create(NULL, NULL);

	custom_widget_layer_status_init(&layer_status_widget, screen);
	lv_obj_align(custom_widget_layer_status_obj(&layer_status_widget), NULL,
		     LV_ALIGN_IN_LEFT_MID, 0, 0);

	return screen;
}
