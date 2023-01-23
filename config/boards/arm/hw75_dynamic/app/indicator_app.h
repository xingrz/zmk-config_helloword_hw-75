/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdint.h>
#include <sys/util_macro.h>

#define INDICATOR_KNOB_CALIBRATING BIT(0)
#define INDICATOR_EINK_UPDATING BIT(1)

void indicator_set_bits(uint32_t bits);
void indicator_clear_bits(uint32_t bits);
