/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#include <knob/pid.h>
#include <knob/time.h>
#include <knob/math.h>

void pid_init(struct pid *pid, float p, float i, float d, float ramp, float limit)
{
	pid->p = p;
	pid->i = i;
	pid->d = d;
	pid->output_ramp = ramp;
	pid->limit = limit;
	pid->error_last = 0;
	pid->output_last = 0;
	pid->integral_last = 0;
	pid->timestamp = time_us();
}

void pid_set(struct pid *pid, float p, float i, float d)
{
	pid->p = p;
	pid->i = i;
	pid->d = d;
}

float pid_regulate(struct pid *pid, float error)
{
	uint32_t time = time_us();
	float dt = (float)(time - pid->timestamp) * 1e-6f;

	// Assuming 0.001s (1ms) if overflowed
	if (dt <= 0 || dt > 0.5f)
		dt = 1e-3f;

	float p = pid->p * error;

	float i = pid->integral_last + pid->i * dt * 0.5f * (error + pid->error_last);
	i = CLAMP(i, -pid->limit, pid->limit);

	float d = pid->d * (error - pid->error_last) / dt;

	float output = p + i + d;
	output = CLAMP(output, -pid->limit, pid->limit);

	// If output ramp defined, limit the acceleration by ramping the output
	if (pid->output_ramp > 0) {
		float output_rate = (output - pid->output_last) / dt;
		if (output_rate > pid->output_ramp) {
			output = pid->output_last + pid->output_ramp * dt;
		} else if (output_rate < -pid->output_ramp) {
			output = pid->output_last - pid->output_ramp * dt;
		}
	}

	pid->integral_last = i;
	pid->output_last = output;
	pid->error_last = error;
	pid->timestamp = time;

	return output;
}
