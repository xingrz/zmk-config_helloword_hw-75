/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_sensor_rotate_mouse_wheel

#include <zephyr/device.h>
#include <drivers/behavior.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zephyr/drivers/sensor.h>
#include <app/hid_mouse.h>

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

static int on_sensor_binding_triggered(struct zmk_behavior_binding *binding,
				       const struct device *sensor, int64_t timestamp)
{
	struct sensor_value value;
	int err;
	int direction;

	err = sensor_channel_get(sensor, SENSOR_CHAN_ROTATION, &value);

	if (err) {
		LOG_WRN("Failed to ge sensor rotation value: %d", err);
		return err;
	}

	switch (value.val1) {
	case 1:
		direction = binding->param1;
		break;
	case -1:
		direction = binding->param2;
		break;
	default:
		return -ENOTSUP;
	}

	return hid_mouse_wheel_report(direction);
}

static const struct behavior_driver_api behavior_sensor_rotate_mouse_wheel_driver_api = {
	.sensor_binding_triggered = on_sensor_binding_triggered
};

static int behavior_sensor_rotate_mouse_wheel_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
};

DEVICE_DT_INST_DEFINE(0, behavior_sensor_rotate_mouse_wheel_init, NULL, NULL, NULL, APPLICATION,
		      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		      &behavior_sensor_rotate_mouse_wheel_driver_api);

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
