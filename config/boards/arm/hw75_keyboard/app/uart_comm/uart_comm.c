/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include <zephyr/drivers/console/uart_slip.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uart_comm, CONFIG_HW75_UART_COMM_LOG_LEVEL);

#include <pb_encode.h>

#include "report/report.h"

#define SLIP_NODE DT_ALIAS(uart_comm)

static uint8_t uart_tx_buf[CONFIG_HW75_UART_COMM_MAX_TX_MESSAGE_SIZE];

static const struct device *slip = DEVICE_DT_GET(SLIP_NODE);

bool uart_comm_report(uart_comm_MessageK2D *k2d)
{
	LOG_DBG("report action: %d", k2d->action);

	pb_ostream_t k2d_stream = pb_ostream_from_buffer(uart_tx_buf, sizeof(uart_tx_buf));

	if (!pb_encode_delimited(&k2d_stream, uart_comm_MessageK2D_fields, k2d)) {
		LOG_ERR("Failed encoding k2d message: %s", k2d_stream.errmsg);
		return false;
	}

	return uart_slip_send(slip, uart_tx_buf, k2d_stream.bytes_written) == 0;
}
