/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_motor

#include <zephyr/device.h>
#include <zephyr/kernel.h>

#include <knob/time.h>
#include <knob/math.h>
#include <knob/lpf.h>
#include <knob/pid.h>
#include <knob/encoder_state.h>
#include <knob/drivers/inverter.h>
#include <knob/drivers/motor.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(motor, CONFIG_ZMK_LOG_LEVEL);

#define MOTOR_VOLTAGE (12.0f)

struct motor_data {
	float voltage_limit;
	float velocity_limit;

	float voltage_limit_calib;

	struct motor_control control;

	struct lpf lpf_velocity;
	struct lpf lpf_angle;
	struct pid pid_velocity;
	struct pid pid_angle;

	float raw_angle;
	float est_angle;
	float raw_velocity;
	float est_velocity;

	struct encoder_state encoder_state;
	enum motor_direction direction;

	float zero_offset;

	bool enable;

	float set_point_voltage;
	float set_point_velocity;
	float set_point_angle;
};

struct motor_config {
	const struct device *inverter;
	const struct device *encoder;
	int pole_pairs;
};

static void motor_close_loop_control_tick(const struct device *dev);
static void motor_foc_output_tick(const struct device *dev);
static void motor_set_phase_voltage(const struct device *dev, float v_q, float v_d, float angle);

int motor_calibrate_set(const struct device *dev, float zero_offset, enum motor_direction direction)
{
	struct motor_data *data = dev->data;
	const struct motor_config *config = dev->config;

	data->zero_offset = zero_offset;
	data->direction = direction;

	encoder_update(&data->encoder_state, config->encoder);
	motor_get_estimate_angle(dev);

	return 0;
}

int motor_calibrate_get(const struct device *dev, float *zero_offset,
			enum motor_direction *direction)
{
	struct motor_data *data = dev->data;
	*zero_offset = data->zero_offset;
	*direction = data->direction;
	return 0;
}

int motor_calibrate_auto(const struct device *dev)
{
	struct motor_data *data = dev->data;
	const struct motor_config *config = dev->config;

	LOG_INF("Calibration start");
	inverter_start(config->inverter);
	k_msleep(100);

	LOG_DBG("Find natual direction");

	for (int i = 0; i <= 500; i++) {
		motor_set_phase_voltage(dev, data->voltage_limit_calib, 0.0f,
					PI3_2 + PI2 * (float)i / 500.0f);
		k_msleep(2);
	}

	encoder_update(&data->encoder_state, config->encoder);
	float mid_angle = encoder_get_full_angle(&data->encoder_state);
	LOG_DBG("Read angle 1: %f (%f deg)", mid_angle, rad_to_deg(mid_angle));

	for (int i = 500; i >= 0; i--) {
		motor_set_phase_voltage(dev, data->voltage_limit_calib, 0.0f,
					PI3_2 + PI2 * (float)i / 500.0f);
		k_msleep(2);
	}

	encoder_update(&data->encoder_state, config->encoder);
	float end_angle = encoder_get_full_angle(&data->encoder_state);
	LOG_DBG("Read angle 2: %f (%f deg)", end_angle, rad_to_deg(end_angle));

	motor_set_phase_voltage(dev, 0.0f, 0.0f, 0.0f);
	k_msleep(200);

	float angle_delta = fabsf(mid_angle - end_angle);
	LOG_DBG("Angle delta: %f (%f deg)", angle_delta, rad_to_deg(angle_delta));

	if (angle_delta <= 0.0f) {
		LOG_ERR("No movement detected");
		inverter_stop(config->inverter);
		return -EIO;
	}

	if (mid_angle < end_angle) {
		data->direction = CCW;
	} else {
		data->direction = CW;
	}

	LOG_INF("Detected encoder direction: %d", data->direction);

	motor_set_phase_voltage(dev, data->voltage_limit_calib, 0.0f, PI3_2);

	k_msleep(1000);

	encoder_update(&data->encoder_state, config->encoder);
	data->zero_offset = 0.0f;
	data->zero_offset = motor_get_electrical_angle(dev);
	LOG_INF("Zero offset: %f (%f deg)", data->zero_offset, rad_to_deg(data->zero_offset));

	inverter_stop(config->inverter);
	LOG_INF("Calibration finished");

	encoder_update(&data->encoder_state, config->encoder);
	motor_get_estimate_angle(dev);

	return 0;
}

bool motor_is_calibrated(const struct device *dev)
{
	struct motor_data *data = dev->data;
	return data->direction != UNKNOWN;
}

void motor_tick(const struct device *dev)
{
	motor_close_loop_control_tick(dev);
	motor_foc_output_tick(dev);
}

static void motor_close_loop_control_tick(const struct device *dev)
{
	struct motor_data *data = dev->data;

	float estimate_angle = motor_get_estimate_angle(dev);
	float estimate_velocity = motor_get_estimate_velocity(dev);

	if (!data->enable)
		return;

	switch (data->control.mode) {
	case TORQUE:
		data->set_point_voltage = data->control.target;
		break;
	case ANGLE:
		data->set_point_angle = data->control.target;
		data->set_point_velocity =
			pid_regulate(&data->pid_angle, data->set_point_angle - estimate_angle);
		data->set_point_voltage = pid_regulate(
			&data->pid_velocity, data->set_point_velocity - estimate_velocity);
		break;
	case VELOCITY:
		data->set_point_velocity = data->control.target;
		data->set_point_voltage = pid_regulate(
			&data->pid_velocity, data->set_point_velocity - estimate_velocity);
		break;
	}
}

static void motor_foc_output_tick(const struct device *dev)
{
	struct motor_data *data = dev->data;
	const struct motor_config *config = dev->config;

	encoder_update(&data->encoder_state, config->encoder);

	if (!data->enable)
		return;

	float electrical_angle = motor_get_electrical_angle(dev) * data->direction;

	float voltage_q = data->set_point_voltage * data->direction;
	float voltage_d = 0.0f;

	motor_set_phase_voltage(dev, voltage_q, voltage_d, electrical_angle);
}

static void motor_set_phase_voltage(const struct device *dev, float v_q, float v_d, float angle)
{
	const struct motor_config *config = dev->config;

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

	inverter_set_powers(config->inverter, tA, tB, tC);
}

void motor_set_enable(const struct device *dev, bool enable)
{
	struct motor_data *data = dev->data;
	const struct motor_config *config = dev->config;

	data->enable = enable;
	if (enable) {
		inverter_start(config->inverter);
	} else {
		inverter_stop(config->inverter);
	}
}

void motor_set_torque_limit(const struct device *dev, float limit)
{
	struct motor_data *data = dev->data;

	data->voltage_limit = CLAMP(limit, limit, MOTOR_VOLTAGE);
	data->pid_velocity.limit = data->voltage_limit;
}

float motor_get_torque_limit(const struct device *dev)
{
	struct motor_data *data = dev->data;

	return data->voltage_limit;
}

void motor_set_angle_pid(const struct device *dev, float p, float i, float d)
{
	struct motor_data *data = dev->data;
	pid_set(&data->pid_angle, p, i, d);
}

void motor_set_velocity_pid(const struct device *dev, float p, float i, float d)
{
	struct motor_data *data = dev->data;
	pid_set(&data->pid_velocity, p, i, d);
}

float motor_get_estimate_angle(const struct device *dev)
{
	struct motor_data *data = dev->data;

	data->raw_angle = encoder_get_full_angle(&data->encoder_state);
	data->est_angle = lpf_apply(&data->lpf_angle, data->raw_angle);

	return data->est_angle;
}

float motor_get_estimate_velocity(const struct device *dev)
{
	struct motor_data *data = dev->data;

	data->raw_velocity = encoder_get_velocity(&data->encoder_state);
	data->est_velocity = lpf_apply(&data->lpf_velocity, data->raw_velocity);

	return data->est_velocity;
}

float motor_get_electrical_angle(const struct device *dev)
{
	struct motor_data *data = dev->data;
	const struct motor_config *config = dev->config;

	return norm_rad((float)config->pole_pairs * encoder_get_lap_angle(&data->encoder_state) -
			data->zero_offset);
}

void motor_reset_rotation_count(const struct device *dev)
{
	struct motor_data *data = dev->data;
	data->encoder_state.rotation_count = 0;
	data->encoder_state.rotation_count_last = 0;
}

struct motor_control *motor_get_control(const struct device *dev)
{
	struct motor_data *data = dev->data;
	return &data->control;
}

void motor_inspect(const struct device *dev, struct motor_state *state)
{
	struct motor_data *data = dev->data;
	state->timestamp = time_us();
	state->control_mode = data->control.mode;
	state->current_angle = data->est_angle;
	state->current_velocity = data->est_velocity;
	state->target_angle = data->set_point_angle;
	state->target_velocity = data->set_point_velocity;
	state->target_voltage = data->set_point_voltage;
}

static int motor_init(const struct device *dev)
{
	struct motor_data *data = dev->data;
	const struct motor_config *config = dev->config;

	if (!device_is_ready(config->inverter)) {
		LOG_ERR("%s: Inverter device is not ready: %s", dev->name, config->inverter->name);
		return -ENODEV;
	}

	if (!device_is_ready(config->encoder)) {
		LOG_ERR("%s: Encoder device is not ready: %s", dev->name, config->encoder->name);
		return -ENODEV;
	}

	data->control.mode = TORQUE;
	data->control.target = 0.0f;

	lpf_init(&data->lpf_velocity, 0.1f);
	lpf_init(&data->lpf_angle, 0.03f);
	pid_init(&data->pid_velocity, 0.1f, 0.0f, 0.0f, 1000.0f, data->voltage_limit);
	pid_init(&data->pid_angle, 80.0f, 0.0f, 0.7f, 0.0f, data->velocity_limit);

	encoder_init(&data->encoder_state, config->encoder);

	return 0;
}

#define MOTOR_INIT(n)                                                                              \
	struct motor_data motor_data_##n = {                                                       \
		.voltage_limit = 1.5f,                                                             \
		.velocity_limit = 100.0f,                                                          \
		.voltage_limit_calib = 1.0f,                                                       \
		.direction = UNKNOWN,                                                              \
		.zero_offset = 0.0f,                                                               \
		.enable = false,                                                                   \
	};                                                                                         \
                                                                                                   \
	struct motor_config motor_config_##n = {                                                   \
		.inverter = DEVICE_DT_GET(DT_INST_PHANDLE(n, inverter)),                           \
		.encoder = DEVICE_DT_GET(DT_INST_PHANDLE(n, encoder)),                             \
		.pole_pairs = DT_INST_PROP(n, pole_pairs),                                         \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, motor_init, NULL, &motor_data_##n, &motor_config_##n,             \
			      POST_KERNEL, CONFIG_KNOB_MOTOR_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(MOTOR_INIT)
