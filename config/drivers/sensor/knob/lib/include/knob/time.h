/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdint.h>
#include <kernel.h>

static inline uint32_t time_us()
{
	return k_ticks_to_us_floor32(k_uptime_ticks());
}
