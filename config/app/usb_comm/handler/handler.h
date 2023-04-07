/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/device.h>

#include "usb_comm.pb.h"

typedef bool (*usb_comm_handler_t)(const usb_comm_MessageH2D *h2d, usb_comm_MessageD2H *d2h,
				   const void *bytes, uint32_t bytes_len);

struct usb_comm_handler_config {
	usb_comm_Action action;
	pb_size_t response_payload;
	usb_comm_handler_t handler;
};

#define USB_COMM_DEFINE_HANDLER(name) static STRUCT_SECTION_ITERABLE(usb_comm_handler_config, name)

#define USB_COMM_HANDLER_DEFINE(_action, _payload, _handler)                                       \
	USB_COMM_DEFINE_HANDLER(usb_comm_handler_##_handler) = {                                   \
		.action = _action,                                                                 \
		.response_payload = _payload,                                                      \
		.handler = _handler,                                                               \
	};
