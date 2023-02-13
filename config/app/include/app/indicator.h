/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

struct indicator_settings {
	bool enable;
	uint8_t brightness_active;
	uint8_t brightness_inactive;
};

uint32_t indicator_set_bits(uint32_t bits);

uint32_t indicator_clear_bits(uint32_t bits);

void indicator_set_enable(bool enable);
void indicator_set_brightness_active(uint8_t brightness);
void indicator_set_brightness_inactive(uint8_t brightness);
const struct indicator_settings *indicator_get_settings(void);
