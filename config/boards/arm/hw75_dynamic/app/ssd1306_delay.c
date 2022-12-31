/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#include <device.h>
#include <kernel.h>

// Delay execution of ssd1306 initialization to allow for power stabilization
// on the device.

// Should be set to a lower value than the CONFIG_DISPLAY_INIT_PRIORITY used by
// the ssd1306 driver to ensure this delay executes before the ssd1306
// initialization.
#define PRE_DISPLAY_INIT_PRIORITY 80

static int ssd1306_delay(const struct device *dev)
{
	ARG_UNUSED(dev);
	k_msleep(100);
	return 0;
}

SYS_INIT(ssd1306_delay, POST_KERNEL, PRE_DISPLAY_INIT_PRIORITY);
