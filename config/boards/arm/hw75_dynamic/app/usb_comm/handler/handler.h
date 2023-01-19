/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "usb_comm.pb.h"

bool handle_version(Version *res);

bool handle_motor_get_state(MotorState *res);
bool handle_knob_get_config(KnobConfig *res);
bool handle_knob_set_config(const KnobConfig *req, KnobConfig *res);
