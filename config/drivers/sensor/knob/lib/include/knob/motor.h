/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <device.h>

#include <knob/lpf.h>
#include <knob/pid.h>
#include <knob/encoder_state.h>

enum motor_control_mode {
	TORQUE,
	VELOCITY,
	ANGLE,
};

struct motor {
	const struct device *encoder;
	const struct device *inverter;

	int pole_pairs;
	float voltage_limit;
	float velocity_limit;

	float voltage_limit_calib;

	enum motor_control_mode control_mode;
	float target;

	struct lpf lpf_velocity;
	struct lpf lpf_angle;
	struct pid pid_velocity;
	struct pid pid_angle;

	float raw_angle;
	float est_angle;
	float raw_velocity;
	float est_velocity;

	struct encoder_state encoder_state;
	enum encoder_direction encoder_dir;

	float zero_offset;

	bool enable;

	float set_point_voltage;
	float set_point_velocity;
	float set_point_angle;
};

void motor_init(struct motor *motor, const struct device *encoder, const struct device *inverter,
		int pole_pairs);

void motor_calibrate_set(struct motor *motor, float zero_offset,
			 enum encoder_direction encoder_dir);
bool motor_calibrate_auto(struct motor *motor);

void motor_set_enable(struct motor *motor, bool enable);
void motor_set_torque_limit(struct motor *motor, float limit);

float motor_get_estimate_angle(struct motor *motor);
float motor_get_estimate_velocity(struct motor *motor);
float motor_get_electrical_angle(struct motor *motor);

void motor_tick(struct motor *motor);
