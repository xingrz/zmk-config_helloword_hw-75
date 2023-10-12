/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#ifndef KNOB_INCLUDE_DRIVERS_KNOB_H_
#define KNOB_INCLUDE_DRIVERS_KNOB_H_

#include <stdbool.h>
#include <zephyr/device.h>

/**
 * @file
 * @brief Extended public API for knob
 */

#ifdef __cplusplus
extern "C" {
#endif

enum knob_mode {
	KNOB_DISABLE = 0,
	KNOB_INERTIA,
	KNOB_ENCODER,
	KNOB_SPRING,
	KNOB_DAMPED,
	KNOB_SPIN,
	KNOB_RATCHET,
	KNOB_SWITCH,
};

struct knob_params {
	int ppr;
};

void knob_set_mode(const struct device *dev, enum knob_mode mode);

enum knob_mode knob_get_mode(const struct device *dev);

void knob_set_enable(const struct device *dev, bool enable);

void knob_set_encoder_report(const struct device *dev, bool report);

bool knob_get_encoder_report(const struct device *dev);

void knob_set_encoder_ppr(const struct device *dev, int ppr);

int knob_get_encoder_ppr(const struct device *dev);

void knob_set_position_limit(const struct device *dev, float min, float max);

void knob_get_position_limit(const struct device *dev, float *min, float *max);

float knob_get_position(const struct device *dev);

float knob_get_velocity(const struct device *dev);

#ifdef __cplusplus
}
#endif

#endif /* KNOB_INCLUDE_DRIVERS_KNOB_H_ */
