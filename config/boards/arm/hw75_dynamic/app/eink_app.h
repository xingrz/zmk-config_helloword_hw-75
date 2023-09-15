/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

int eink_update(const uint8_t *image, uint32_t image_len, bool partial);

int eink_update_region(const uint8_t *image, uint32_t image_len, uint32_t x, uint32_t y,
		       uint32_t width, uint32_t height, bool partial);
