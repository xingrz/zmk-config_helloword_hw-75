/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/devicetree.h>

#include "handler.h"
#include "usb_comm.pb.h"

#include <pb_encode.h>

#ifdef VER_ZEPHYR
static const char *zephyr_version = VER_ZEPHYR;
#else
static const char *zephyr_version = "unknown";
#endif

#ifdef VER_ZMK
static const char *zmk_version = VER_ZMK;
#else
static const char *zmk_version = "unknown";
#endif

#ifdef VER_APP
static const char *app_version = VER_APP;
#else
static const char *app_version = "unknown";
#endif

static bool write_string(pb_ostream_t *stream, const pb_field_t *field, void *const *arg)
{
	char *str = *arg;
	if (!pb_encode_tag_for_field(stream, field)) {
		return false;
	}
	return pb_encode_string(stream, (uint8_t *)str, strlen(str));
}

static bool handle_version(const usb_comm_MessageH2D *h2d, usb_comm_MessageD2H *d2h,
			   const void *bytes, uint32_t bytes_len)
{
	usb_comm_Version *res = &d2h->payload.version;
	res->zephyr_version.funcs.encode = write_string;
	res->zephyr_version.arg = (void *)zephyr_version;
	res->zmk_version.funcs.encode = write_string;
	res->zmk_version.arg = (void *)zmk_version;
	res->app_version.funcs.encode = write_string;
	res->app_version.arg = (void *)app_version;

	res->has_features = true;

#ifdef CONFIG_HW75_USB_COMM_FEATURE_RGB
	res->features.has_rgb = res->features.rgb = true;
	res->features.has_rgb_full_control = res->features.rgb_full_control = true;
	res->features.has_rgb_indicator = res->features.rgb_indicator = true;
#endif // CONFIG_HW75_USB_COMM_FEATURE_RGB

#ifdef CONFIG_HW75_USB_COMM_FEATURE_EINK
	res->features.has_eink = res->features.eink = true;
#endif // CONFIG_HW75_USB_COMM_FEATURE_EINK

#ifdef CONFIG_HW75_USB_COMM_FEATURE_KNOB
	res->features.has_knob = res->features.knob = true;
	res->features.has_knob_prefs = res->features.knob_prefs = true;
#endif // CONFIG_HW75_USB_COMM_FEATURE_KNOB

#if DT_HAS_COMPAT_STATUS_OKAY(zmk_knob_profile_switch)
	res->features.has_knob_profile_switch = res->features.knob_profile_switch = true;
#endif // DT_HAS_COMPAT_STATUS_OKAY(zmk_knob_profile_switch)

	res->features.has_knob_spring_report = res->features.knob_spring_report = true;

	return true;
}

USB_COMM_HANDLER_DEFINE(usb_comm_Action_VERSION, usb_comm_MessageD2H_version_tag, handle_version);
