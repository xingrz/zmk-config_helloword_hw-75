/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#include <kernel.h>
#include <string.h>

#include <knob/time.h>
#include <knob/math.h>
#include <knob/encoder_state.h>
#include <knob/drivers/encoder.h>

void encoder_init(struct encoder_state *state, const struct device *dev)
{
	memset(state, 0, sizeof(struct encoder_state));

	// Pre-initialize encoder data to avoid jumping from zero

	encoder_get_radian(dev);
	k_usleep(1);

	state->velocity_last = encoder_get_radian(dev);
	state->velocity_time = time_us();

	k_msleep(1);

	encoder_get_radian(dev);
	k_usleep(1);

	state->angle_last = encoder_get_radian(dev);
	state->angle_time = time_us();
}

void encoder_update(struct encoder_state *state, const struct device *dev)
{
	float angle = encoder_get_radian(dev);
	state->angle_time = time_us();

	float angle_delta = angle - state->angle_last;

	// If overflow happened track it as full rotation
	if (fabsf(angle_delta) > (0.8f * PI2)) {
		state->rotation_count += (angle_delta > 0) ? -1 : 1;
	}

	state->angle_last = angle;
}

float encoder_get_lap_angle(struct encoder_state *state)
{
	return state->angle_last;
}

float encoder_get_full_angle(struct encoder_state *state)
{
	return (float)state->rotation_count * PI2 + state->angle_last;
}

float encoder_get_velocity(struct encoder_state *state)
{
	int32_t rotation_count_delta = state->rotation_count - state->rotation_count_last;
	float angle_delta = state->angle_last - state->velocity_last;
	float time_delta = (float)(state->angle_time - state->velocity_time) * 1e-6f;

	// Assuming 0.001s (1ms) if overflowed
	if (time_delta <= 0)
		time_delta = 1e-3f;

	float velocity = ((float)rotation_count_delta * PI2 + angle_delta) / time_delta;

	state->velocity_last = state->angle_last;
	state->rotation_count_last = state->rotation_count;
	state->velocity_time = state->angle_time;

	return velocity;
}
