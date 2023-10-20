/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <math.h>
#include <arm_math.h>

/**
 * @brief π
 */
#ifndef PI
#define PI 3.14159265359f
#endif

/**
 * @brief 2π
 */
#ifndef PI2
#define PI2 6.28318530718f
#endif

/**
 * @brief π/2
 */
#ifndef PI_2
#define PI_2 1.57079632679f
#endif

/**
 * @brief π/3
 */
#ifndef PI_3
#define PI_3 1.0471975512f
#endif

/**
 * @brief 3π/2
 */
#ifndef PI3_2
#define PI3_2 4.71238898038f
#endif

/**
 * @brief √3
 */
#ifndef SQRT3
#define SQRT3 1.73205080757f
#endif

/**
 * @brief Normalize a radian
 */
static inline float norm_rad(float radian)
{
	float r = fmod(radian, PI2);
	return r >= 0 ? r : (r + PI2);
}

/**
 * @brief Radian to degree
 */
#define rad_to_deg(rad) (360.0f * (rad / PI2))

/**
 * @brief Degree to radian
 */
#define deg_to_rad(deg) ((float)deg / 360.0f * PI2)
