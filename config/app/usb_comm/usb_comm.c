/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#include <kernel.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(usb_comm, CONFIG_HW75_USB_COMM_LOG_LEVEL);

#include <sys/byteorder.h>
#include <usb/usb_device.h>
#include <usb_descriptor.h>

#include "usb_comm_bos.h"
#include "usb_comm_proto.h"

#define USB_IN_EP_IDX 0
#define USB_OUT_EP_IDX 1

#define USB_BULK_EP_MPS 64

USBD_CLASS_DESCR_DEFINE(primary, 0) struct {
	struct usb_if_descriptor if0;
	struct usb_ep_descriptor if0_in_ep;
	struct usb_ep_descriptor if0_out_ep;
} usb_comm_desc = {
	.if0 = {
		.bLength = sizeof(struct usb_if_descriptor),
		.bDescriptorType = USB_DESC_INTERFACE,
		.bInterfaceNumber = 0,
		.bAlternateSetting = 0,
		.bNumEndpoints = 2,
		.bInterfaceClass = USB_BCC_VENDOR,
		.bInterfaceSubClass = 0,
		.bInterfaceProtocol = 0,
		.iInterface = 0,
	},
	.if0_in_ep = {
		.bLength = sizeof(struct usb_ep_descriptor),
		.bDescriptorType = USB_DESC_ENDPOINT,
		.bEndpointAddress = AUTO_EP_IN,
		.bmAttributes = USB_DC_EP_BULK,
		.wMaxPacketSize = sys_cpu_to_le16(USB_BULK_EP_MPS),
		.bInterval = 0x00,
	},
	.if0_out_ep = {
		.bLength = sizeof(struct usb_ep_descriptor),
		.bDescriptorType = USB_DESC_ENDPOINT,
		.bEndpointAddress = AUTO_EP_OUT,
		.bmAttributes = USB_DC_EP_BULK,
		.wMaxPacketSize = sys_cpu_to_le16(USB_BULK_EP_MPS),
		.bInterval = 0x00,
	},
};

static struct usb_ep_cfg_data usb_comm_ep_data[] = {
	{ .ep_cb = usb_transfer_ep_callback, .ep_addr = AUTO_EP_IN },
	{ .ep_cb = usb_transfer_ep_callback, .ep_addr = AUTO_EP_OUT },
};

static void usb_comm_status_cb(struct usb_cfg_data *cfg, enum usb_dc_status_code status,
			       const uint8_t *param)
{
	ARG_UNUSED(cfg);

	switch (status) {
	case USB_DC_CONFIGURED:
		LOG_DBG("USB device configured");
		usb_comm_ready(cfg->endpoint[USB_IN_EP_IDX].ep_addr,
			       cfg->endpoint[USB_OUT_EP_IDX].ep_addr);
		break;
	default:
		break;
	}
}

USBD_DEFINE_CFG_DATA(usb_config) = {
	.usb_device_description = NULL,
	.interface_descriptor = &usb_comm_desc.if0,
	.cb_usb_status = usb_comm_status_cb,
	.interface = {
		.class_handler = NULL,
		.custom_handler = usb_comm_custom_handler,
		.vendor_handler = usb_comm_vendor_handler,
	},
	.num_endpoints = ARRAY_SIZE(usb_comm_ep_data),
	.endpoint = usb_comm_ep_data,
};
