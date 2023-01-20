/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#include "handler.h"
#include "usb_comm.pb.h"

#include <zmk/rgb_underglow.h>

bool handle_rgb_control(const RgbControl *control, RgbState *state)
{
	switch (control->command) {
	case RgbControl_Command_RGB_ON:
		zmk_rgb_underglow_on();
		break;
	case RgbControl_Command_RGB_OFF:
		zmk_rgb_underglow_off();
		break;
	case RgbControl_Command_RGB_HUI:
		zmk_rgb_underglow_change_hue(1);
		break;
	case RgbControl_Command_RGB_HUD:
		zmk_rgb_underglow_change_hue(-1);
		break;
	case RgbControl_Command_RGB_SAI:
		zmk_rgb_underglow_change_sat(1);
		break;
	case RgbControl_Command_RGB_SAD:
		zmk_rgb_underglow_change_sat(-1);
		break;
	case RgbControl_Command_RGB_BRI:
		zmk_rgb_underglow_change_brt(1);
		break;
	case RgbControl_Command_RGB_BRD:
		zmk_rgb_underglow_change_brt(-1);
		break;
	case RgbControl_Command_RGB_SPI:
		zmk_rgb_underglow_change_spd(1);
		break;
	case RgbControl_Command_RGB_SPD:
		zmk_rgb_underglow_change_spd(-1);
		break;
	case RgbControl_Command_RGB_EFF:
		zmk_rgb_underglow_cycle_effect(1);
		break;
	case RgbControl_Command_RGB_EFR:
		zmk_rgb_underglow_cycle_effect(-1);
		break;
	}

	return handle_rgb_get_state(state);
}

bool handle_rgb_get_state(RgbState *state)
{
	bool on;
	if (zmk_rgb_underglow_get_state(&on) != 0) {
		return false;
	}

	state->on = on;

	return true;
}
