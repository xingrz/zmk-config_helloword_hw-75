/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#include "layer_status.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/display.h>
#include <zmk/keymap.h>
#include <zmk/event_manager.h>
#include <zmk/events/layer_state_changed.h>

#include "icons.h"

#define DISPLAY_NODE DT_CHOSEN(zephyr_display)

#define SCREEN_W DT_PROP(DISPLAY_NODE, width)
#define SCREEN_H DT_PROP(DISPLAY_NODE, height)
#define ITEM_SIZE MIN(SCREEN_W, SCREEN_H)

#define KEYMAP_NODE DT_INST(0, zmk_keymap)

#define LAYER_CHILD_LEN(node) 1 +
#define KEYMAP_LAYERS_NUM (DT_FOREACH_CHILD(KEYMAP_NODE, LAYER_CHILD_LEN) 0)

#define LAYER_LABEL(node)                                                                          \
	COND_CODE_0(DT_NODE_HAS_PROP(node, label), (NULL), (DT_PROP(node, label))),

LV_FONT_DECLARE(mono_19);
LV_FONT_DECLARE(zfull_9);

static const char *layer_icons[KEYMAP_LAYERS_NUM] = {
	ICON_VOL,
	ICON_SCR,
	ICON_ARROW_VER,
	ICON_ARROW_HOR,
};

static const char *layer_names[KEYMAP_LAYERS_NUM] = { DT_FOREACH_CHILD(KEYMAP_NODE, LAYER_LABEL) };

static lv_obj_t *layer_name;
static lv_obj_t *layer_list;
static lv_obj_t *layer_items[KEYMAP_LAYERS_NUM] = {};

static lv_style_t st_name;
static lv_style_t st_list;
static lv_style_t st_item;
static lv_style_t st_item_focused;

struct layer_status_state {
	uint8_t index;
};

static void update_layer_status(struct layer_status_state *state)
{
	lv_group_focus_obj(layer_items[state->index]);
	lv_label_set_text(layer_name, layer_names[state->index]);
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

static void layer_button_selected(lv_event_t *event)
{
	int layer_id = (int)event->user_data;
	if (layer_id != zmk_keymap_highest_layer_active()) {
		zmk_keymap_layer_to((uint8_t)layer_id);
	}
}

int layer_status_init(lv_obj_t *parent, lv_group_t *group)
{
	lv_style_init(&st_name);
	lv_style_set_pad_ver(&st_name, 4);
	lv_style_set_text_font(&st_name, &zfull_9);
	lv_style_set_text_align(&st_name, LV_TEXT_ALIGN_CENTER);

	lv_style_init(&st_list);
	lv_style_set_flex_grow(&st_list, 1);

	lv_style_init(&st_item);
	lv_style_set_radius(&st_item, 3);
	lv_style_set_flex_main_place(&st_item, LV_FLEX_ALIGN_CENTER);
	lv_style_set_flex_track_place(&st_item, LV_FLEX_ALIGN_CENTER);
	lv_style_set_bg_opa(&st_item, LV_OPA_0);
	lv_style_set_bg_color(&st_item, lv_color_black());
	lv_style_set_text_color(&st_item, lv_color_black());
	lv_style_set_text_font(&st_item, &mono_19);

	lv_style_init(&st_item_focused);
	lv_style_set_bg_opa(&st_item_focused, LV_OPA_100);
	lv_style_set_text_color(&st_item_focused, lv_color_white());

	lv_obj_t *cont = lv_obj_create(parent);
	lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
	lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);

	lv_obj_t *name = layer_name = lv_label_create(cont);
	lv_obj_add_style(name, &st_name, LV_PART_MAIN);
	lv_obj_set_size(name, LV_PCT(100), LV_SIZE_CONTENT);

	lv_obj_t *list = layer_list = lv_list_create(cont);
	lv_obj_add_style(list, &st_list, LV_PART_MAIN);

	for (int i = 0; i < KEYMAP_LAYERS_NUM; i++) {
		lv_obj_t *btn = layer_items[i] = lv_list_add_btn(list, NULL, layer_icons[i]);
		lv_obj_add_style(btn, &st_item, LV_PART_MAIN);
		lv_obj_add_style(btn, &st_item_focused, LV_PART_MAIN | LV_STATE_FOCUSED);
		lv_obj_set_size(btn, ITEM_SIZE, ITEM_SIZE);
		lv_obj_add_event_cb(btn, layer_button_selected, LV_EVENT_FOCUSED, (void *)i);

		lv_obj_t *label = lv_obj_get_child(btn, 0);
		lv_obj_set_flex_grow(label, 0);

		lv_group_add_obj(group, btn);
	}

	layer_status_subscribtion_init();

	return 0;
}
