/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/class/usb_hid.h>

#include <zmk/usb.h>

#include <app/hid_mouse.h>

static const uint8_t hid_mouse_report_desc[] = HID_MOUSE_REPORT_DESC(2);

static const struct device *hid_dev;

static K_SEM_DEFINE(hid_sem, 1, 1);

static void in_ready_cb(const struct device *dev)
{
	ARG_UNUSED(dev);
	k_sem_give(&hid_sem);
}

static const struct hid_ops ops = {
	.int_in_ready = in_ready_cb,
};

static int hid_mouse_send_report(const uint8_t *report, size_t len)
{
	switch (zmk_usb_get_status()) {
	case USB_DC_SUSPEND:
		return usb_wakeup_request();
	case USB_DC_ERROR:
	case USB_DC_RESET:
	case USB_DC_DISCONNECTED:
	case USB_DC_UNKNOWN:
		return -ENODEV;
	default:
		k_sem_take(&hid_sem, K_MSEC(30));
		int err = hid_int_ep_write(hid_dev, report, len, NULL);

		if (err) {
			k_sem_give(&hid_sem);
		}

		return err;
	}
}

int hid_mouse_wheel_report(int direction, bool pressed)
{
	uint8_t val = pressed ? (uint8_t)(direction & 0xFF) : 0x00;

	uint8_t report[] = { 0x00, 0x00, 0x00, val };
	hid_mouse_send_report(report, sizeof(report));

	return 0;
}

int hid_mouse_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	hid_dev = device_get_binding(CONFIG_HW75_HID_MOUSE_DEVICE_NAME);
	if (hid_dev == NULL) {
		LOG_ERR("Unable to locate HID device");
		return -ENODEV;
	}

	usb_hid_register_device(hid_dev, hid_mouse_report_desc, sizeof(hid_mouse_report_desc),
				&ops);
	usb_hid_init(hid_dev);

	return 0;
}

SYS_INIT(hid_mouse_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
