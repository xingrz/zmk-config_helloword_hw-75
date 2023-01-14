/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#include <kernel.h>

#include <knob/motor.h>
#include <knob/math.h>
#include <knob/drivers/inverter.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(motor, CONFIG_ZMK_LOG_LEVEL);

#define MOTOR_VOLTAGE (12.0f)

void motor_close_loop_control_tick(struct motor *motor);
void motor_foc_output_tick(struct motor *motor);
void motor_set_phase_voltage(struct motor *motor, float v_q, float v_d, float angle);

void motor_init(struct motor *motor, const struct device *encoder, const struct device *inverter,
		int pole_pairs)
{
	memset(motor, 0, sizeof(struct motor));

	motor->encoder = encoder;
	motor->inverter = inverter;

	motor->pole_pairs = pole_pairs;

	motor->voltage_limit = MOTOR_VOLTAGE;
	motor->velocity_limit = 20.0f;
	motor->voltage_limit_calib = 1.0f;

	motor->control_mode = ANGLE;
	motor->target = 0.0f;

	lpf_init(&motor->lpf_velocity, 0.1f);
	lpf_init(&motor->lpf_angle, 0.03f);
	pid_init(&motor->pid_velocity, 0.0f, 10.0f, 0.0f, 1000.0f, motor->voltage_limit);
	pid_init(&motor->pid_angle, 20.0f, 0.0f, 0.0f, 0.0f, motor->velocity_limit);

	motor->encoder_dir = UNKNOWN;
	encoder_init(&motor->encoder_state, encoder);

	motor->zero_offset = 0.0f;
	motor->enable = false;
}

void motor_calibrate_set(struct motor *motor, float zero_offset, enum encoder_direction encoder_dir)
{
	motor->zero_offset = zero_offset;
	motor->encoder_dir = encoder_dir;

	encoder_update(&motor->encoder_state, motor->encoder);
	motor_get_estimate_angle(motor);
}

bool motor_calibrate_auto(struct motor *motor)
{
	LOG_INF("Calibration start");
	inverter_start(motor->inverter);
	k_msleep(100);

	LOG_DBG("Find natual direction");

	for (int i = 0; i <= 500; i++) {
		motor_set_phase_voltage(motor, motor->voltage_limit_calib, 0.0f,
					PI3_2 + PI2 * (float)i / 500.0f);
		k_msleep(2);
	}

	encoder_update(&motor->encoder_state, motor->encoder);
	float mid_angle = encoder_get_full_angle(&motor->encoder_state);
	LOG_DBG("Read angle 1: %f (%f deg)", mid_angle, rad_to_deg(mid_angle));

	for (int i = 500; i >= 0; i--) {
		motor_set_phase_voltage(motor, motor->voltage_limit_calib, 0.0f,
					PI3_2 + PI2 * (float)i / 500.0f);
		k_msleep(2);
	}

	encoder_update(&motor->encoder_state, motor->encoder);
	float end_angle = encoder_get_full_angle(&motor->encoder_state);
	LOG_DBG("Read angle 2: %f (%f deg)", end_angle, rad_to_deg(end_angle));

	motor_set_phase_voltage(motor, 0.0f, 0.0f, 0.0f);
	k_msleep(200);

	float angle_delta = fabsf(mid_angle - end_angle);
	LOG_DBG("Angle delta: %f (%f deg)", angle_delta, rad_to_deg(angle_delta));

	if (angle_delta <= 0.0f) {
		LOG_ERR("No movement detected");
		inverter_stop(motor->inverter);
		return false;
	}

	if (mid_angle < end_angle) {
		motor->encoder_dir = CCW;
	} else {
		motor->encoder_dir = CW;
	}

	LOG_INF("Detected encoder direction: %d", motor->encoder_dir);

	motor_set_phase_voltage(motor, motor->voltage_limit_calib, 0.0f, PI3_2);

	k_msleep(1000);

	encoder_update(&motor->encoder_state, motor->encoder);
	motor->zero_offset = 0.0f;
	motor->zero_offset = motor_get_electrical_angle(motor);
	LOG_INF("Zero offset: %f (%f deg)", motor->zero_offset, rad_to_deg(motor->zero_offset));

	inverter_stop(motor->inverter);
	LOG_INF("Calibration finished");

	encoder_update(&motor->encoder_state, motor->encoder);
	motor_get_estimate_angle(motor);

	return true;
}

void motor_tick(struct motor *motor)
{
	motor_close_loop_control_tick(motor);
	motor_foc_output_tick(motor);
}

void motor_close_loop_control_tick(struct motor *motor)
{
	float estimate_angle = motor_get_estimate_angle(motor);
	float estimate_velocity = motor_get_estimate_velocity(motor);

	if (!motor->enable)
		return;

	switch (motor->control_mode) {
	case TORQUE:
		motor->set_point_voltage = motor->target;
		break;
	case ANGLE:
		motor->set_point_angle = motor->target;
		motor->set_point_velocity =
			pid_regulate(&motor->pid_angle, motor->set_point_angle - estimate_angle);
		motor->set_point_voltage = pid_regulate(
			&motor->pid_velocity, motor->set_point_velocity - estimate_velocity);
		break;
	case VELOCITY:
		motor->set_point_velocity = motor->target;
		motor->set_point_voltage = pid_regulate(
			&motor->pid_velocity, motor->set_point_velocity - estimate_velocity);
		break;
	}
}

void motor_foc_output_tick(struct motor *motor)
{
	encoder_update(&motor->encoder_state, motor->encoder);

	if (!motor->enable)
		return;

	float electrical_angle = motor_get_electrical_angle(motor);

	float voltage_q = motor->set_point_voltage;
	float voltage_d = 0.0f;

	motor_set_phase_voltage(motor, voltage_q, voltage_d, electrical_angle);
}

void motor_set_phase_voltage(struct motor *motor, float v_q, float v_d, float angle)
{
	float mod;

	if (v_d != 0) {
		arm_sqrt_f32(v_d * v_d + v_q * v_q, &mod);
		angle = norm_rad(angle + atan2f(v_q, v_d));
	} else {
		mod = v_q;
		angle = norm_rad(angle + PI_2);
	}

	mod = mod / MOTOR_VOLTAGE;

	uint8_t sec = (int)(floorf(angle / PI_3)) + 1;

	float t1 = SQRT3 * arm_sin_f32((float)sec * PI_3 - angle) * mod;
	float t2 = SQRT3 * arm_sin_f32(angle - ((float)sec - 1.0f) * PI_3) * mod;
	float t0 = 1 - t1 - t2;

	float tA, tB, tC;
	switch (sec) {
	case 1:
		tA = t1 + t2 + t0 / 2;
		tB = t2 + t0 / 2;
		tC = t0 / 2;
		break;
	case 2:
		tA = t1 + t0 / 2;
		tB = t1 + t2 + t0 / 2;
		tC = t0 / 2;
		break;
	case 3:
		tA = t0 / 2;
		tB = t1 + t2 + t0 / 2;
		tC = t2 + t0 / 2;
		break;
	case 4:
		tA = t0 / 2;
		tB = t1 + t0 / 2;
		tC = t1 + t2 + t0 / 2;
		break;
	case 5:
		tA = t2 + t0 / 2;
		tB = t0 / 2;
		tC = t1 + t2 + t0 / 2;
		break;
	case 6:
		tA = t1 + t2 + t0 / 2;
		tB = t0 / 2;
		tC = t1 + t0 / 2;
		break;
	default:
		tA = 0;
		tB = 0;
		tC = 0;
	}

	inverter_set_powers(motor->inverter, tA, tB, tC);
}

void motor_set_enable(struct motor *motor, bool enable)
{
	motor->enable = enable;
	if (enable) {
		inverter_start(motor->inverter);
	} else {
		inverter_stop(motor->inverter);
	}
}

void motor_set_torque_limit(struct motor *motor, float limit)
{
	motor->voltage_limit = CLAMP(limit, limit, MOTOR_VOLTAGE);
	motor->pid_velocity.limit = motor->voltage_limit;
}

float motor_get_estimate_angle(struct motor *motor)
{
	motor->raw_angle =
		(float)motor->encoder_dir * encoder_get_full_angle(&motor->encoder_state);
	motor->est_angle = lpf_apply(&motor->lpf_angle, motor->raw_angle);
	return motor->est_angle;
}

float motor_get_estimate_velocity(struct motor *motor)
{
	motor->raw_velocity =
		(float)motor->encoder_dir * encoder_get_velocity(&motor->encoder_state);
	motor->est_velocity = lpf_apply(&motor->lpf_velocity, motor->raw_velocity);
	return motor->est_velocity;
}

float motor_get_electrical_angle(struct motor *motor)
{
	return norm_rad((float)motor->encoder_dir * (float)motor->pole_pairs *
				encoder_get_lap_angle(&motor->encoder_state) -
			motor->zero_offset);
}
