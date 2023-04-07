/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_lvgl_key_press

#include <zephyr/device.h>
#include <drivers/behavior.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/behavior.h>
#include <drivers/behavior/lvgl_key_press.h>

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#define LVKP_MSGQ_COUNT 10
K_MSGQ_DEFINE(lvkp_msgq, sizeof(lv_indev_data_t), LVKP_MSGQ_COUNT, 4);

static lv_indev_drv_t indev_drv;
static lv_indev_t *indev = NULL;

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
				     struct zmk_behavior_binding_event event)
{
	if (!indev) {
		return 0;
	}

	lv_indev_data_t data = {
		.key = binding->param1,
		.state = LV_INDEV_STATE_PRESSED,
	};

	int ret = k_msgq_put(&lvkp_msgq, &data, K_NO_WAIT);
	if (ret != 0) {
		LOG_ERR("Could put input data into queue: %s", strerror(ret));
	}

	return ret;
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
				      struct zmk_behavior_binding_event event)
{
	if (!indev) {
		return 0;
	}

	lv_indev_data_t data = {
		.key = binding->param1,
		.state = LV_INDEV_STATE_RELEASED,
	};

	int ret = k_msgq_put(&lvkp_msgq, &data, K_NO_WAIT);
	if (ret != 0) {
		LOG_ERR("Could put input data into queue: %s", strerror(ret));
	}

	return ret;
}

static const struct behavior_driver_api behavior_lvgl_key_press_driver_api = {
	.binding_pressed = on_keymap_binding_pressed,
	.binding_released = on_keymap_binding_released
};

static void indev_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
	ARG_UNUSED(drv);

	if (k_msgq_get(&lvkp_msgq, data, K_NO_WAIT) == 0) {
		LOG_DBG("Got key %d: %d", data->key, data->state);
	}

	data->continue_reading = k_msgq_num_used_get(&lvkp_msgq) > 0;
}

lv_indev_t *behavior_lvgl_get_indev(void)
{
	if (!indev) {
		lv_indev_drv_init(&indev_drv);
		indev_drv.type = LV_INDEV_TYPE_KEYPAD;
		indev_drv.read_cb = indev_read_cb;
		indev = lv_indev_drv_register(&indev_drv);
	}

	return indev;
}

static int behavior_lvgl_key_press_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
};

DEVICE_DT_INST_DEFINE(0, behavior_lvgl_key_press_init, NULL, NULL, NULL, APPLICATION,
		      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_lvgl_key_press_driver_api);

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
