/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#include <zmk/display/status_screen.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include "layer_status.h"

lv_obj_t *zmk_display_status_screen()
{
	lv_obj_t *screen;

	screen = lv_obj_create(NULL, NULL);

	layer_status_init(screen);

	return screen;
}
