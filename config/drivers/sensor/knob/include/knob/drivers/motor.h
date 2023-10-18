/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#ifndef KNOB_INCLUDE_DRIVERS_MOTOR_H_
#define KNOB_INCLUDE_DRIVERS_MOTOR_H_

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/device.h>

/**
 * @file
 * @brief Extended public API for motor
 */

#ifdef __cplusplus
extern "C" {
#endif

enum motor_control_mode {
	TORQUE,
	VELOCITY,
	ANGLE,
};

struct motor_control {
	enum motor_control_mode mode;
	float target;
};

enum motor_direction {
	CW = 1,
	CCW = -1,
	UNKNOWN = 0,
};

int motor_calibrate_set(const struct device *dev, float zero_offset,
			enum motor_direction direction);

int motor_calibrate_auto(const struct device *dev);

bool motor_is_calibrated(const struct device *dev);

void motor_tick(const struct device *dev);

void motor_set_enable(const struct device *dev, bool enable);

void motor_set_torque_limit(const struct device *dev, float limit);

float motor_get_torque_limit(const struct device *dev);

void motor_set_angle_pid(const struct device *dev, float p, float i, float d);

void motor_set_velocity_pid(const struct device *dev, float p, float i, float d);

float motor_get_estimate_angle(const struct device *dev);

float motor_get_estimate_velocity(const struct device *dev);

float motor_get_electrical_angle(const struct device *dev);

void motor_reset_rotation_count(const struct device *dev);

struct motor_control *motor_get_control(const struct device *dev);

struct motor_state {
	uint32_t timestamp;
	enum motor_control_mode control_mode;
	float current_angle;
	float current_velocity;
	float target_angle;
	float target_velocity;
	float target_voltage;
};

void motor_inspect(const struct device *dev, struct motor_state *state);

#ifdef __cplusplus
}
#endif

#endif /* KNOB_INCLUDE_DRIVERS_MOTOR_H_ */
