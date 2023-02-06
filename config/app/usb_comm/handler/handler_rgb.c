/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#include "handler.h"
#include "usb_comm.pb.h"

#include <zmk/rgb_underglow.h>

bool handle_rgb_control(const usb_comm_MessageH2D *h2d, usb_comm_MessageD2H *d2h, const void *bytes,
			uint32_t bytes_len)
{
	const usb_comm_RgbControl *req = &h2d->payload.rgb_control;

	switch (req->command) {
	case usb_comm_RgbControl_Command_RGB_ON:
		zmk_rgb_underglow_on();
		break;
	case usb_comm_RgbControl_Command_RGB_OFF:
		zmk_rgb_underglow_off();
		break;
	case usb_comm_RgbControl_Command_RGB_HUI:
		zmk_rgb_underglow_change_hue(1);
		break;
	case usb_comm_RgbControl_Command_RGB_HUD:
		zmk_rgb_underglow_change_hue(-1);
		break;
	case usb_comm_RgbControl_Command_RGB_SAI:
		zmk_rgb_underglow_change_sat(1);
		break;
	case usb_comm_RgbControl_Command_RGB_SAD:
		zmk_rgb_underglow_change_sat(-1);
		break;
	case usb_comm_RgbControl_Command_RGB_BRI:
		zmk_rgb_underglow_change_brt(1);
		break;
	case usb_comm_RgbControl_Command_RGB_BRD:
		zmk_rgb_underglow_change_brt(-1);
		break;
	case usb_comm_RgbControl_Command_RGB_SPI:
		zmk_rgb_underglow_change_spd(1);
		break;
	case usb_comm_RgbControl_Command_RGB_SPD:
		zmk_rgb_underglow_change_spd(-1);
		break;
	case usb_comm_RgbControl_Command_RGB_EFF:
		zmk_rgb_underglow_cycle_effect(1);
		break;
	case usb_comm_RgbControl_Command_RGB_EFR:
		zmk_rgb_underglow_cycle_effect(-1);
		break;
	}

	return handle_rgb_get_state(h2d, d2h, NULL, 0);
}

bool handle_rgb_get_state(const usb_comm_MessageH2D *h2d, usb_comm_MessageD2H *d2h,
			  const void *bytes, uint32_t bytes_len)
{
	usb_comm_RgbState *res = &d2h->payload.rgb_state;

	bool on;
	if (zmk_rgb_underglow_get_state(&on) != 0) {
		return false;
	}

	res->on = on;

	return true;
}
