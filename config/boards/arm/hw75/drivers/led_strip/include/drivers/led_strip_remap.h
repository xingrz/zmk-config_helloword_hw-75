/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

/**
 * @file
 * @brief Extended public API for led_strip_remap
 */

#pragma once

#include <drivers/led_strip.h>

#ifdef __cplusplus
extern "C" {
#endif

int led_strip_remap_set(const struct device *dev, const char *label, struct led_rgb *pixel);

int led_strip_remap_clear(const struct device *dev, const char *label);

#ifdef __cplusplus
}
#endif
