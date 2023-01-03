/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#ifndef KNOB_INCLUDE_DRIVERS_ENCODER_H_
#define KNOB_INCLUDE_DRIVERS_ENCODER_H_

#include <device.h>

/**
 * @defgroup knob_drivers_encoder Encoder API
 * @ingroup knob_drivers
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

struct encoder_driver_api {
	float (*get_radian)(const struct device *dev);
};

/** @endcond */

/**
 * @brief Get angle
 *
 * @param dev Encoder instance
 * @return Radian
 */
static inline float encoder_get_radian(const struct device *dev)
{
	const struct encoder_driver_api *api = (const struct encoder_driver_api *)dev->api;

	return api->get_radian(dev);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* KNOB_INCLUDE_DRIVERS_ENCODER_H_ */
