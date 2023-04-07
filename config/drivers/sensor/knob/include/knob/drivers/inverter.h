/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#ifndef KNOB_INCLUDE_DRIVERS_INVERTER_H_
#define KNOB_INCLUDE_DRIVERS_INVERTER_H_

#include <zephyr/device.h>

/**
 * @defgroup knob_drivers_inverter Inverter API
 * @ingroup knob_drivers
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

struct inverter_driver_api {
	void (*start)(const struct device *dev);
	void (*stop)(const struct device *dev);
	void (*set_powers)(const struct device *dev, float a, float b, float c);
};

/** @endcond */

/**
 * @brief Start the inverter controller
 *
 * @param dev Inverter instance
 */
static inline void inverter_start(const struct device *dev)
{
	const struct inverter_driver_api *api = (const struct inverter_driver_api *)dev->api;

	return api->start(dev);
}

/**
 * @brief Stop the inverter controller
 *
 * @param dev Inverter instance
 */
static inline void inverter_stop(const struct device *dev)
{
	const struct inverter_driver_api *api = (const struct inverter_driver_api *)dev->api;

	return api->stop(dev);
}

/**
 * @brief Set the power for each phase of the output, with values normalized from 0.0f to 1.0f.
 *
 * @param dev Inverter instance
 * @param a Power for phase A
 * @param b Power for phase B
 * @param c Power for phase C
 */
static inline void inverter_set_powers(const struct device *dev, float a, float b, float c)
{
	const struct inverter_driver_api *api = (const struct inverter_driver_api *)dev->api;

	return api->set_powers(dev, a, b, c);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* KNOB_INCLUDE_DRIVERS_INVERTER_H_ */
