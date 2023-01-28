/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "usb_comm.pb.h"

typedef bool (*usb_comm_handler_t)(const usb_comm_MessageH2D *h2d, usb_comm_MessageD2H *d2h,
				   const void *bytes, uint32_t bytes_len);

bool handle_version(const usb_comm_MessageH2D *h2d, usb_comm_MessageD2H *d2h, const void *bytes,
		    uint32_t bytes_len);

bool handle_motor_get_state(const usb_comm_MessageH2D *h2d, usb_comm_MessageD2H *d2h,
			    const void *bytes, uint32_t bytes_len);
bool handle_knob_get_config(const usb_comm_MessageH2D *h2d, usb_comm_MessageD2H *d2h,
			    const void *bytes, uint32_t bytes_len);
bool handle_knob_set_config(const usb_comm_MessageH2D *h2d, usb_comm_MessageD2H *d2h,
			    const void *bytes, uint32_t bytes_len);

bool handle_rgb_control(const usb_comm_MessageH2D *h2d, usb_comm_MessageD2H *d2h, const void *bytes,
			uint32_t bytes_len);
bool handle_rgb_get_state(const usb_comm_MessageH2D *h2d, usb_comm_MessageD2H *d2h,
			  const void *bytes, uint32_t bytes_len);

bool handle_eink_set_image(const usb_comm_MessageH2D *h2d, usb_comm_MessageD2H *d2h,
			   const void *bytes, uint32_t bytes_len);
