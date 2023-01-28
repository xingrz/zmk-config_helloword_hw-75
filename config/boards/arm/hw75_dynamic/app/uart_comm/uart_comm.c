/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#include <kernel.h>
#include <device.h>

#include <drivers/console/uart_slip.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(uart_comm, CONFIG_HW75_UART_COMM_LOG_LEVEL);

#define SLIP_LABEL DT_LABEL(DT_NODELABEL(slip))

static uint8_t uart_rx_buf[CONFIG_HW75_UART_COMM_MAX_RX_MESSAGE_SIZE];

K_THREAD_STACK_MEMBER(thread_stack, CONFIG_HW75_UART_COMM_THREAD_STACK_SIZE);
struct k_thread thread;

static const struct device *slip;

static void uart_comm_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	uint32_t len;

	while (1) {
		uart_slip_receive(slip, uart_rx_buf, sizeof(uart_rx_buf), &len);
		LOG_HEXDUMP_DBG(uart_rx_buf, len, "RX");
	}
}

static int uart_comm_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	slip = device_get_binding(SLIP_LABEL);
	if (!slip) {
		LOG_ERR("SLIP device not found");
		return -ENODEV;
	}

	k_thread_create(&thread, thread_stack, CONFIG_HW75_UART_COMM_THREAD_STACK_SIZE,
			(k_thread_entry_t)uart_comm_thread, NULL, NULL, NULL,
			K_PRIO_COOP(CONFIG_HW75_UART_COMM_THREAD_PRIORITY), 0, K_NO_WAIT);

	return 0;
}

SYS_INIT(uart_comm_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
