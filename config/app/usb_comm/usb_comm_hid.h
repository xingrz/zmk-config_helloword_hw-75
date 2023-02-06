/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdint.h>

typedef void (*usb_comm_receive_callback_t)(uint8_t *data, uint32_t len);

int usb_comm_hid_init(usb_comm_receive_callback_t callback);
int usb_comm_hid_send(uint8_t *data, uint32_t len);
