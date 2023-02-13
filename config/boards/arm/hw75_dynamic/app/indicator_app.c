/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#include <kernel.h>
#include <device.h>
#include <sys/util_macro.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/event_manager.h>
#include <app/events/knob_state_changed.h>
#include <app/events/eink_state_changed.h>

#include <app/indicator.h>

enum indicator_flags {
	INDICATOR_KNOB_CALIBRATING,
	INDICATOR_EINK_UPDATING,
};

static int indicator_app_event_listener(const zmk_event_t *eh)
{
	struct app_knob_state_changed *knob_ev;
	struct app_eink_state_changed *eink_ev;

	if ((knob_ev = as_app_knob_state_changed(eh)) != NULL) {
		if (knob_ev->calibration == KNOB_CALIBRATE_OK) {
			indicator_clear_bits(BIT(INDICATOR_KNOB_CALIBRATING));
		} else {
			indicator_set_bits(BIT(INDICATOR_KNOB_CALIBRATING));
		}
		return 0;
	} else if ((eink_ev = as_app_eink_state_changed(eh)) != NULL) {
		if (eink_ev->busy) {
			indicator_set_bits(BIT(INDICATOR_EINK_UPDATING));
		} else {
			indicator_clear_bits(BIT(INDICATOR_EINK_UPDATING));
		}
		return 0;
	}

	return -ENOTSUP;
}

ZMK_LISTENER(indicator_app, indicator_app_event_listener);
ZMK_SUBSCRIPTION(indicator_app, app_knob_state_changed);
ZMK_SUBSCRIPTION(indicator_app, app_eink_state_changed);
