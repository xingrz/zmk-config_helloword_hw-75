/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usb_comm, CONFIG_HW75_USB_COMM_LOG_LEVEL);

#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/class/usb_hid.h>

#include "usb_comm_hid.h"

#define HID_COMM_REPORT_ID (1)
#define HID_COMM_REPORT_COUNT (63)

static const struct device *hid_dev;
static usb_comm_receive_callback_t receive_callback;

static K_SEM_DEFINE(hid_sem, 1, 1);

#define HID_USAGE_PAGE_VENDOR_DEFINED(page)                                                        \
	HID_ITEM(HID_ITEM_TAG_USAGE_PAGE, HID_ITEM_TYPE_GLOBAL, 2), page, 0xFF

static const uint8_t hid_comm_report_desc[] = {
	HID_USAGE_PAGE_VENDOR_DEFINED(0x14),

	HID_USAGE(0x01),
	HID_COLLECTION(HID_COLLECTION_APPLICATION),
	HID_LOGICAL_MIN8(0x00),
	HID_LOGICAL_MAX8(0xFF),
	HID_REPORT_SIZE(8),
	HID_REPORT_COUNT(HID_COMM_REPORT_COUNT),

	HID_REPORT_ID(HID_COMM_REPORT_ID),
	HID_USAGE(0x02),
	HID_INPUT(0x86),

	HID_REPORT_ID(HID_COMM_REPORT_ID),
	HID_USAGE(0x02),
	HID_OUTPUT(0x86),

	HID_END_COLLECTION,
};

static int hid_comm_set_report_cb(const struct device *dev, struct usb_setup_packet *setup,
				  int32_t *len, uint8_t **data)
{
	LOG_DBG("packet size %u", *len);
	LOG_HEXDUMP_DBG(*data, *len, "packet data");
	if (*data[0] == HID_COMM_REPORT_ID) {
		receive_callback(*data + 1, *len - 1);
		return 0;
	} else {
		return -ENOTSUP;
	}
}

static void hid_comm_in_ready_cb(const struct device *dev)
{
	ARG_UNUSED(dev);
	k_sem_give(&hid_sem);
}

static const struct hid_ops ops = {
	.set_report = hid_comm_set_report_cb,
	.int_in_ready = hid_comm_in_ready_cb,
};

int usb_comm_hid_init(usb_comm_receive_callback_t callback)
{
	hid_dev = device_get_binding(CONFIG_HW75_USB_COMM_DEVICE_NAME);
	if (hid_dev == NULL) {
		LOG_ERR("Unable to locate HID device");
		return -ENODEV;
	}

	usb_hid_register_device(hid_dev, hid_comm_report_desc, sizeof(hid_comm_report_desc), &ops);

	receive_callback = callback;

	return usb_hid_init(hid_dev);
}

int usb_comm_hid_send(uint8_t *data, uint32_t len)
{
	int ret;

	LOG_DBG("message size %u", len);
	LOG_HEXDUMP_DBG(data, MIN(len, 64), "message data");

	static uint8_t tx_buf[HID_COMM_REPORT_COUNT + 1];
	uint32_t tx_len;

	uint32_t written;

	tx_buf[0] = HID_COMM_REPORT_ID;
	do {
		tx_len = MIN(HID_COMM_REPORT_COUNT - 1, len);
		tx_buf[1] = tx_len & 0xFF;
		memcpy(tx_buf + 2, data, tx_len);
		data += tx_len;
		len -= tx_len;

		k_sem_take(&hid_sem, K_MSEC(30));

		LOG_DBG("packet size %u", sizeof(tx_buf));
		LOG_HEXDUMP_DBG(tx_buf, sizeof(tx_buf), "packet data");

		ret = hid_int_ep_write(hid_dev, tx_buf, sizeof(tx_buf), &written);
		if (ret != 0) {
			k_sem_give(&hid_sem);
			LOG_ERR("HID write failed: %d", ret);
			return ret;
		}
		if (written != sizeof(tx_buf)) {
			LOG_ERR("HID write corrupted, requested %u, sent %u", sizeof(tx_buf),
				written);
			return -EIO;
		}
	} while (len);

	return 0;
}
