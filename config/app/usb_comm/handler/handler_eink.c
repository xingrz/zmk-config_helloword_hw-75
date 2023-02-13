/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#include "handler.h"
#include "usb_comm.pb.h"

#include <device.h>
#include <drivers/display.h>
#include <drivers/display/ssd16xx.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <eink_app.h>

#include <pb_encode.h>
#include <pb_decode.h>

bool handle_eink_set_image(const usb_comm_MessageH2D *h2d, usb_comm_MessageD2H *d2h,
			   const void *bytes, uint32_t bytes_len)
{
	const usb_comm_EinkImage *req = &h2d->payload.eink_image;
	usb_comm_EinkImage *res = &d2h->payload.eink_image;

	res->id = req->id;

	eink_update(bytes, bytes_len);

	return true;
}
