/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdint.h>

struct lpf {
	uint32_t timestamp;
	float output_last;
	float time_constant;
};

void lpf_init(struct lpf *lpf, float time_constant);
float lpf_apply(struct lpf *lpf, float input);
