/*
 * Copyright (c) 2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#ifndef KNOB_INCLUDE_DRIVERS_PROFILE_H_
#define KNOB_INCLUDE_DRIVERS_PROFILE_H_

#include <zephyr/device.h>
#include <knob/drivers/motor.h>

#define KNOB_PROFILE_CFG_ROM                                                                       \
	const struct device *knob;                                                                 \
	const struct device *motor;

#define KNOB_PROFILE_CFG_INIT                                                                      \
	.knob = DEVICE_DT_GET(DT_INST_PARENT(0)),                                                  \
	.motor = DEVICE_DT_GET(DT_PHANDLE(DT_INST_PARENT(0), motor)),

#define KNOB_PROFILE_TORQUE_LIMIT ((float)DT_INST_PROP(0, torque_limit_mv) / 1000.0f)

#define Z_KNOB_PROFILE_PID(prop)                                                                   \
	((float)DT_INST_PROP_BY_IDX(0, prop, 0) / 1000.0f),                                        \
		((float)DT_INST_PROP_BY_IDX(0, prop, 1) / 1000.0f),                                \
		((float)DT_INST_PROP_BY_IDX(0, prop, 2) / 1000.0f)

#define KNOB_PROFILE_HAS_VELOCITY_PID DT_INST_NODE_HAS_PROP(0, velocity_pid)
#define KNOB_PROFILE_VELOCITY_PID Z_KNOB_PROFILE_PID(velocity_pid)

#define KNOB_PROFILE_HAS_ANGLE_PID DT_INST_NODE_HAS_PROP(0, angle_pid)
#define KNOB_PROFILE_ANGLE_PID Z_KNOB_PROFILE_PID(angle_pid)

#ifdef __cplusplus
extern "C" {
#endif

struct knob_profile_api {
	int (*enable)(const struct device *dev);
	int (*update_params)(const struct device *dev, struct knob_params params);
	int (*tick)(const struct device *dev, struct motor_control *mc);
	int (*report)(const struct device *dev, int32_t *val);
};

static inline int knob_profile_enable(const struct device *dev)
{
	const struct knob_profile_api *api = dev->api;

	if (api->enable == NULL) {
		return -ENOTSUP;
	}

	return api->enable(dev);
}

static inline int knob_profile_update_params(const struct device *dev, struct knob_params params)
{
	const struct knob_profile_api *api = dev->api;

	if (api->update_params == NULL) {
		return -ENOTSUP;
	}

	return api->update_params(dev, params);
}

static inline int knob_profile_tick(const struct device *dev, struct motor_control *mc)
{
	const struct knob_profile_api *api = dev->api;

	if (api->tick == NULL) {
		return -ENOTSUP;
	}

	return api->tick(dev, mc);
}

static inline int knob_profile_report(const struct device *dev, int32_t *val)
{
	const struct knob_profile_api *api = dev->api;

	if (api->report == NULL) {
		return -ENOTSUP;
	}

	return api->report(dev, val);
}

#ifdef __cplusplus
}
#endif

#endif /* KNOB_INCLUDE_DRIVERS_PROFILE_H_ */
