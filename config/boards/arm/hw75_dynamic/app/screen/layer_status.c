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

#define DISPLAY_NODE DT_CHOSEN(zephyr_display)

#define SCREEN_W DT_PROP(DISPLAY_NODE, width)
#define SCREEN_H DT_PROP(DISPLAY_NODE, height)
#define ITEM_SIZE MIN(SCREEN_W, SCREEN_H)

#define KEYMAP_NODE DT_INST(0, zmk_keymap)

#define LAYER_CHILD_LEN(node) 1 +
#define KEYMAP_LAYERS_NUM (DT_FOREACH_CHILD(KEYMAP_NODE, LAYER_CHILD_LEN) 0)

#define LAYER_LABEL(node) COND_CODE_0(DT_NODE_HAS_PROP(node, label), (NULL), (DT_LABEL(node))),

static const char *layer_names[KEYMAP_LAYERS_NUM] = { DT_FOREACH_CHILD(KEYMAP_NODE, LAYER_LABEL) };

static lv_obj_t *layer_list;
static lv_obj_t *layer_items[KEYMAP_LAYERS_NUM] = {};

LV_FONT_DECLARE(icons_19);

static lv_style_t st_list;
static lv_style_t st_item;

struct layer_status_state {
	uint8_t index;
};

static void update_layer_status(struct layer_status_state *state)
{
	lv_list_focus_btn(layer_list, layer_items[state->index]);
	lv_list_focus(layer_items[state->index], LV_ANIM_ON);
}

static void layer_status_update_cb(struct layer_status_state state)
{
	update_layer_status(&state);
}

static struct layer_status_state layer_status_get_state(const zmk_event_t *eh)
{
	uint8_t index = zmk_keymap_highest_layer_active();
	return (struct layer_status_state){ .index = index };
}

ZMK_DISPLAY_WIDGET_LISTENER(layer_status_subscribtion, struct layer_status_state,
			    layer_status_update_cb, layer_status_get_state)

ZMK_SUBSCRIPTION(layer_status_subscribtion, zmk_layer_state_changed);

int layer_status_init(lv_obj_t *parent)
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
	lv_style_set_text_font(&st_item, LV_STATE_DEFAULT, &icons_19);

	lv_obj_t *list = layer_list = lv_list_create(parent, NULL);
	lv_list_set_layout(list, LV_LAYOUT_COLUMN_MID);
	lv_obj_add_style(list, LV_OBJ_PART_MAIN, &st_list);
	lv_obj_set_size(list, SCREEN_W, SCREEN_H);

	for (int i = 0; i < KEYMAP_LAYERS_NUM; i++) {
		lv_obj_t *btn = layer_items[i] = lv_list_add_btn(list, NULL, layer_names[i]);
		lv_obj_add_style(btn, LV_BTN_PART_MAIN, &st_item);
		lv_btn_set_fit(btn, LV_FIT_NONE);
		lv_obj_set_size(btn, ITEM_SIZE, ITEM_SIZE);
		lv_obj_t *label = lv_list_get_btn_label(btn);
		lv_label_set_align(label, LV_LABEL_ALIGN_CENTER);
	}

	layer_status_subscribtion_init();

	return 0;
}
