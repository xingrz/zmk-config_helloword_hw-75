/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#include <kernel.h>
#include <device.h>

#include "report.h"

#define PING_REPORT_INTERVAL_MS 5000

static void report_ping_tick(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(report_ping_work, report_ping_tick);

static void report_ping_tick(struct k_work *work)
{
	uart_comm_MessageK2D k2d = uart_comm_MessageK2D_init_zero;
	k2d.action = uart_comm_Action_PING;
	k2d.which_payload = uart_comm_MessageK2D_nop_tag;

	uart_comm_report(&k2d);

	k_work_schedule(&report_ping_work, K_MSEC(PING_REPORT_INTERVAL_MS));
}

static int report_ping_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	report_ping_tick(&report_ping_work.work);

	return 0;
}

SYS_INIT(report_ping_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
