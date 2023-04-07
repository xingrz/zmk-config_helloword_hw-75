/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include <zephyr/drivers/console/uart_slip.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uart_comm, CONFIG_HW75_UART_COMM_LOG_LEVEL);

#include <pb_decode.h>

#include "handler/handler.h"

#define SLIP_NODE DT_ALIAS(uart_comm)

static uint8_t uart_rx_buf[CONFIG_HW75_UART_COMM_MAX_RX_MESSAGE_SIZE];

K_THREAD_STACK_MEMBER(thread_stack, CONFIG_HW75_UART_COMM_THREAD_STACK_SIZE);
struct k_thread thread;

static const struct device *slip = DEVICE_DT_GET(SLIP_NODE);

static struct {
	uart_comm_Action action;
	uart_comm_handler_t handler;
} handlers[] = {
	{ uart_comm_Action_FN_STATE_CHANGED, handle_fn_state },
};

static void uart_comm_handle(uint32_t len)
{
	LOG_HEXDUMP_DBG(uart_rx_buf, len, "RX");

	pb_istream_t k2d_stream = pb_istream_from_buffer(uart_rx_buf, len);

	uart_comm_MessageK2D k2d = uart_comm_MessageK2D_init_zero;
	if (!pb_decode_delimited(&k2d_stream, uart_comm_MessageK2D_fields, &k2d)) {
		LOG_ERR("Failed decoding k2d message: %s", k2d_stream.errmsg);
		return;
	}

	LOG_DBG("report action: %d", k2d.action);

	for (size_t i = 0; i < ARRAY_SIZE(handlers); i++) {
		if (handlers[i].action == k2d.action) {
			handlers[i].handler(&k2d);
			break;
		}
	}
}

static void uart_comm_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	uint32_t len;

	while (1) {
		uart_slip_receive(slip, uart_rx_buf, sizeof(uart_rx_buf), &len);
		uart_comm_handle(len);
	}
}

static int uart_comm_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	k_thread_create(&thread, thread_stack, CONFIG_HW75_UART_COMM_THREAD_STACK_SIZE,
			(k_thread_entry_t)uart_comm_thread, NULL, NULL, NULL,
			K_PRIO_COOP(CONFIG_HW75_UART_COMM_THREAD_PRIORITY), 0, K_NO_WAIT);

	return 0;
}

SYS_INIT(uart_comm_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
