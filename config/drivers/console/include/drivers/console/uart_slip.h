/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

/**
 * @file
 * @brief A transport layer built on top of UART
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief Encode and send data over UART
 *
 * @param[in] dev   Device instance
 * @param[in] buf   Pointer to data to send
 * @param[in] len   Number of bytes to send
 *
 * @return 0 on success or negative error
 */
int uart_slip_send(const struct device *dev, const uint8_t *buf, uint32_t len);

/**
 * @brief Receive and decode data from UART
 *
 * @param[in] dev   Device instance
 * @param[in] buf   Pointer to data buffer to write to
 * @param[in] limit Max length of data to receive
 * @param[out] len  Number of bytes received
 *
 * @return 0 on success or negative error
 */
int uart_slip_receive(const struct device *dev, uint8_t *buf, uint32_t limit, uint32_t *len);

#ifdef __cplusplus
}
#endif
