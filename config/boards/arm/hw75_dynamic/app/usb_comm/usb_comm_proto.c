/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#include <kernel.h>
#include <device.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(usb_comm, CONFIG_HW75_USB_COMM_LOG_LEVEL);

#include <usb/usb_device.h>

#include <pb_encode.h>
#include <pb_decode.h>

#include "usb_comm_proto.h"
#include "usb_comm.pb.h"

#include "handler/handler.h"

static uint8_t usb_ep_in, usb_ep_out;
static uint8_t usb_rx_buf[CONFIG_HW75_USB_COMM_MAX_RX_PACKET_SIZE];
static uint8_t usb_tx_buf[CONFIG_HW75_USB_COMM_MAX_TX_PACKET_SIZE];

static void usb_comm_read(void);

static void usb_comm_proto_write_cb(uint8_t ep, int size, void *priv);
static void usb_comm_proto_read_cb(uint8_t ep, int size, void *priv);

void usb_comm_ready(uint8_t ep_in, uint8_t ep_out)
{
	usb_ep_in = ep_in;
	usb_ep_out = ep_out;
	usb_comm_read();
}

static void usb_comm_proto_write_cb(uint8_t ep, int size, void *priv)
{
	LOG_DBG("ep %x, size %u", ep, size);
	LOG_HEXDUMP_DBG(usb_tx_buf, size, "tx");
}

#define D2H_PAYLOAD_OR_NOP(result, tag) (result ? tag : MessageD2H_nop_tag)

static void usb_comm_proto_read_cb(uint8_t ep, int size, void *priv)
{
	LOG_DBG("ep %x, size %u", ep, size);
	LOG_HEXDUMP_DBG(usb_rx_buf, size, "rx");

	pb_istream_t h2d_stream = pb_istream_from_buffer(usb_rx_buf, size);
	pb_ostream_t d2h_stream = pb_ostream_from_buffer(usb_tx_buf, sizeof(usb_tx_buf));

	MessageH2D h2d = MessageH2D_init_zero;
	MessageD2H d2h = MessageD2H_init_zero;

	if (!pb_decode_delimited(&h2d_stream, MessageH2D_fields, &h2d)) {
		LOG_ERR("Failed decoding h2d message: %s", h2d_stream.errmsg);
		goto next;
	}

	LOG_DBG("req action: %d", h2d.action);
	d2h.action = h2d.action;

	switch (h2d.action) {
	case Action_VERSION:
		d2h.which_payload = D2H_PAYLOAD_OR_NOP(handle_version(&d2h.payload.version),
						       MessageD2H_version_tag);
		break;
	case Action_MOTOR_GET_STATE:
		d2h.which_payload =
			D2H_PAYLOAD_OR_NOP(handle_motor_get_state(&d2h.payload.motor_state),
					   MessageD2H_motor_state_tag);
		break;
	case Action_KNOB_GET_CONFIG:
		d2h.which_payload =
			D2H_PAYLOAD_OR_NOP(handle_knob_get_config(&d2h.payload.knob_config),
					   MessageD2H_knob_config_tag);
		break;
	case Action_KNOB_SET_CONFIG:
		d2h.which_payload = D2H_PAYLOAD_OR_NOP(
			handle_knob_set_config(&h2d.payload.knob_config, &d2h.payload.knob_config),
			MessageD2H_knob_config_tag);
		break;
	default:
		d2h.which_payload = MessageD2H_nop_tag;
		break;
	}

	size_t d2h_size;
	pb_get_encoded_size(&d2h_size, MessageD2H_fields, &d2h);
	if (d2h_size > sizeof(usb_tx_buf)) {
		LOG_ERR("The size of response for action %d is %d, exceeds max packet size %d",
			h2d.action, d2h_size, sizeof(usb_tx_buf));
	}

	if (!pb_encode_delimited(&d2h_stream, MessageD2H_fields, &d2h)) {
		LOG_ERR("Failed encoding d2h message: %s", d2h_stream.errmsg);
		goto next;
	}

	usb_transfer(usb_ep_in, usb_tx_buf, d2h_stream.bytes_written, USB_TRANS_WRITE,
		     usb_comm_proto_write_cb, NULL);

next:
	usb_comm_read();
}

static void usb_comm_read(void)
{
	usb_transfer(usb_ep_out, usb_rx_buf, sizeof(usb_rx_buf), USB_TRANS_READ,
		     usb_comm_proto_read_cb, NULL);
}
