/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <lvgl.h>
#include <kernel.h>

#define KEYMAP_NODE DT_INST(0, zmk_keymap)

#define LAYER_CHILD_LEN(node) 1 +
#define KEYMAP_LAYERS_NUM (DT_FOREACH_CHILD(KEYMAP_NODE, LAYER_CHILD_LEN) 0)

struct custom_widget_layer_status {
	sys_snode_t node;
	lv_obj_t *obj;
};

int custom_widget_layer_status_init(struct custom_widget_layer_status *widget, lv_obj_t *parent);
lv_obj_t *custom_widget_layer_status_obj(struct custom_widget_layer_status *widget);
