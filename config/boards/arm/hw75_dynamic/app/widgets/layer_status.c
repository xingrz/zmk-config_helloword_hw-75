/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#include "layer_status.h"

#include <kernel.h>
#include <device.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/display.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/endpoints.h>
#include <zmk/keymap.h>

#define SCREEN_W 128
#define SCREEN_H 32
#define ITEM_SIZE 32

#define KEYMAP_NODE DT_INST(0, zmk_keymap)

#define LAYER_CHILD_LEN(node) 1 +
#define KEYMAP_LAYERS_NUM (DT_FOREACH_CHILD(KEYMAP_NODE, LAYER_CHILD_LEN) 0)

#define LAYER_LABEL(node) COND_CODE_0(DT_NODE_HAS_PROP(node, label), (NULL), (DT_LABEL(node))),

static const char *layer_names[KEYMAP_LAYERS_NUM] = { DT_FOREACH_CHILD(KEYMAP_NODE, LAYER_LABEL) };

static lv_obj_t *layer_items[KEYMAP_LAYERS_NUM] = {};

static lv_style_t st_list;
static lv_style_t st_item;

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct layer_status_state {
	uint8_t index;
};

static void update_layer_status(struct custom_widget_layer_status *widget,
				struct layer_status_state state)
{
	lv_list_focus_btn(widget->obj, layer_items[state.index]);
	lv_list_focus(layer_items[state.index], LV_ANIM_ON);
}

static void layer_status_update_cb(struct layer_status_state state)
{
	struct custom_widget_layer_status *widget;
	SYS_SLIST_FOR_EACH_CONTAINER (&widgets, widget, node) {
		update_layer_status(widget, state);
	}
}

static struct layer_status_state layer_status_get_state(const zmk_event_t *eh)
{
	uint8_t index = zmk_keymap_highest_layer_active();
	return (struct layer_status_state){ .index = index };
}

ZMK_DISPLAY_WIDGET_LISTENER(custom_layer_status, struct layer_status_state, layer_status_update_cb,
			    layer_status_get_state)

ZMK_SUBSCRIPTION(custom_layer_status, zmk_layer_state_changed);

int custom_widget_layer_status_init(struct custom_widget_layer_status *widget, lv_obj_t *parent)
{
	lv_style_init(&st_list);
	lv_style_set_radius(&st_list, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_ver(&st_list, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_hor(&st_list, LV_STATE_DEFAULT, 0);
	lv_style_set_margin_all(&st_list, LV_STATE_DEFAULT, 0);

	lv_style_init(&st_item);
	lv_style_set_radius(&st_item, LV_STATE_DEFAULT, 2);
	lv_style_set_border_width(&st_item, LV_STATE_DEFAULT, 0);
	lv_style_set_border_color(&st_item, LV_STATE_DEFAULT, LV_COLOR_BLACK);
	lv_style_set_margin_all(&st_item, LV_STATE_DEFAULT, 0);
	lv_style_set_bg_opa(&st_item, LV_STATE_DEFAULT, LV_OPA_100);
	lv_style_set_bg_color(&st_item, LV_STATE_DEFAULT, LV_COLOR_TRANSP);
	lv_style_set_text_color(&st_item, LV_STATE_DEFAULT, LV_COLOR_BLACK);
	lv_style_set_bg_color(&st_item, LV_STATE_FOCUSED, LV_COLOR_BLACK);
	lv_style_set_text_color(&st_item, LV_STATE_FOCUSED, LV_COLOR_WHITE);

	lv_obj_t *list = widget->obj = lv_list_create(parent, NULL);
	lv_list_set_layout(list, LV_LAYOUT_ROW_MID);
	lv_obj_add_style(list, LV_OBJ_PART_MAIN, &st_list);
	lv_obj_set_size(list, SCREEN_W, SCREEN_H);

	LOG_ERR("layers: %d", KEYMAP_LAYERS_NUM);
	for (int i = 0; i < KEYMAP_LAYERS_NUM; i++) {
		lv_obj_t *btn = layer_items[i] = lv_list_add_btn(list, NULL, layer_names[i]);
		lv_obj_add_style(btn, LV_BTN_PART_MAIN, &st_item);
		lv_btn_set_fit(btn, LV_FIT_NONE);
		lv_obj_set_size(btn, ITEM_SIZE, ITEM_SIZE);
		lv_obj_t *label = lv_list_get_btn_label(btn);
		lv_label_set_align(label, LV_LABEL_ALIGN_CENTER);
	}

	sys_slist_append(&widgets, &widget->node);

	custom_layer_status_init();
	return 0;
}

lv_obj_t *custom_widget_layer_status_obj(struct custom_widget_layer_status *widget)
{
	return widget->obj;
}
