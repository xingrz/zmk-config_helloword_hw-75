/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdint.h>

struct pid {
	float p;
	float i;
	float d;
	float output_ramp;
	float limit;
	float error_last;
	float output_last;
	float integral_last;
	uint32_t timestamp;
};

void pid_init(struct pid *pid, float p, float i, float d, float ramp, float limit);
void pid_set(struct pid *pid, float p, float i, float d);
float pid_regulate(struct pid *pid, float error);
