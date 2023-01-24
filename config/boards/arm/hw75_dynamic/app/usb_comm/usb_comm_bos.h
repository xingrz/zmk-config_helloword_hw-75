/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdint.h>
#include <usb/usb_device.h>

int usb_comm_custom_handler(struct usb_setup_packet *setup, int32_t *len, uint8_t **data);
int usb_comm_vendor_handler(struct usb_setup_packet *setup, int32_t *len, uint8_t **data);
