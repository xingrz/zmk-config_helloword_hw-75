/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#include "handler.h"
#include "usb_comm.pb.h"

#include <zmk/rgb_underglow.h>
#include <app/indicator.h>

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

	if (zmk_rgb_underglow_get_state(&res->on) != 0) {
		return false;
	}

	struct zmk_led_hsb color = zmk_rgb_underglow_calc_hue(0);
	res->color.h = color.h;
	res->color.s = color.s;
	res->color.b = color.b;
	res->has_color = true;

	res->effect = zmk_rgb_underglow_calc_effect(0);
	res->has_effect = true;

	return true;
}

bool handle_rgb_set_state(const usb_comm_MessageH2D *h2d, usb_comm_MessageD2H *d2h,
			  const void *bytes, uint32_t bytes_len)
{
	const usb_comm_RgbState *req = &h2d->payload.rgb_state;

	if (req->on) {
		zmk_rgb_underglow_on();
	} else {
		zmk_rgb_underglow_off();
	}

	if (req->has_color) {
		zmk_rgb_underglow_set_hsb((struct zmk_led_hsb){
			.h = req->color.h,
			.s = req->color.s,
			.b = req->color.b,
		});
	}

	if (req->has_effect) {
		zmk_rgb_underglow_select_effect((int)req->effect);
	}

	return handle_rgb_get_state(h2d, d2h, bytes, bytes_len);
}

bool handle_rgb_get_indicator(const usb_comm_MessageH2D *h2d, usb_comm_MessageD2H *d2h,
			      const void *bytes, uint32_t bytes_len)
{
	usb_comm_RgbIndicator *res = &d2h->payload.rgb_indicator;

	const struct indicator_settings *settings = indicator_get_settings();

	res->enable = settings->enable;
	res->has_enable = true;
	res->brightness_active = settings->brightness_active;
	res->has_brightness_active = true;
	res->brightness_inactive = settings->brightness_inactive;
	res->has_brightness_inactive = true;

	return true;
}

bool handle_rgb_set_indicator(const usb_comm_MessageH2D *h2d, usb_comm_MessageD2H *d2h,
			      const void *bytes, uint32_t bytes_len)
{
	const usb_comm_RgbIndicator *req = &h2d->payload.rgb_indicator;

	if (req->has_enable) {
		indicator_set_enable(req->enable);
	}
	if (req->has_brightness_active) {
		indicator_set_brightness_active(req->brightness_active);
	}
	if (req->has_brightness_inactive) {
		indicator_set_brightness_inactive(req->brightness_inactive);
	}

	return handle_rgb_get_indicator(h2d, d2h, bytes, bytes_len);
}
