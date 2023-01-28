/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#include <kernel.h>
#include <device.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(usb_comm, CONFIG_HW75_USB_COMM_LOG_LEVEL);

#include <usb/usb_device.h>

#include <pb_encode.h>
#include <pb_decode.h>

#include "usb_comm_proto.h"
#include "usb_comm.pb.h"

#include "handler/handler.h"

static uint8_t usb_ep_in, usb_ep_out;
static uint8_t usb_rx_buf[CONFIG_HW75_USB_COMM_MAX_RX_MESSAGE_SIZE];
static uint8_t usb_tx_buf[CONFIG_HW75_USB_COMM_MAX_TX_MESSAGE_SIZE];

static uint8_t bytes_field[CONFIG_HW75_USB_COMM_MAX_BYTES_FIELD_SIZE];
static uint32_t bytes_field_len = 0;

static struct {
	usb_comm_Action action;
	pb_size_t which_payload;
	usb_comm_handler_t handler;
} handlers[] = {
	{ usb_comm_Action_VERSION, usb_comm_MessageD2H_version_tag, handle_version },
	{ usb_comm_Action_MOTOR_GET_STATE, usb_comm_MessageD2H_motor_state_tag,
	  handle_motor_get_state },
	{ usb_comm_Action_KNOB_GET_CONFIG, usb_comm_MessageD2H_knob_config_tag,
	  handle_knob_get_config },
	{ usb_comm_Action_KNOB_SET_CONFIG, usb_comm_MessageD2H_knob_config_tag,
	  handle_knob_set_config },
	{ usb_comm_Action_RGB_CONTROL, usb_comm_MessageD2H_rgb_state_tag, handle_rgb_control },
	{ usb_comm_Action_RGB_GET_STATE, usb_comm_MessageD2H_rgb_state_tag, handle_rgb_get_state },
	{ usb_comm_Action_EINK_SET_IMAGE, usb_comm_MessageD2H_eink_image_tag,
	  handle_eink_set_image },
};

static bool read_bytes_field(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
	ARG_UNUSED(field);
	ARG_UNUSED(arg);

	if (stream->bytes_left > sizeof(bytes_field)) {
		LOG_ERR("Buffer overflows decoding %d bytes", stream->bytes_left);
		return false;
	}

	uint32_t bytes_len = stream->bytes_left;

	if (!pb_read(stream, bytes_field, stream->bytes_left)) {
		LOG_ERR("Failed decoding bytes: %s", stream->errmsg);
		return false;
	}

	bytes_field_len = bytes_len;
	LOG_DBG("Decoded %d bytes", bytes_field_len);

	return true;
}

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
	LOG_HEXDUMP_DBG(usb_tx_buf, MIN(size, 64), "tx");
}

#define D2H_PAYLOAD_OR_NOP(result, tag) (result ? tag : MessageD2H_nop_tag)

static bool h2d_callback(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
	if (field->tag == usb_comm_MessageH2D_eink_image_tag) {
		usb_comm_EinkImage *eink_image = field->pData;
		eink_image->bits.funcs.decode = read_bytes_field;
	}
	return true;
}

static void usb_comm_proto_read_cb(uint8_t ep, int size, void *priv)
{
	LOG_DBG("ep %x, size %u", ep, size);
	LOG_HEXDUMP_DBG(usb_rx_buf, MIN(size, 64), "rx");

	pb_istream_t h2d_stream = pb_istream_from_buffer(usb_rx_buf, size);
	pb_ostream_t d2h_stream = pb_ostream_from_buffer(usb_tx_buf, sizeof(usb_tx_buf));

	usb_comm_MessageH2D h2d = usb_comm_MessageH2D_init_zero;
	usb_comm_MessageD2H d2h = usb_comm_MessageD2H_init_zero;

	h2d.cb_payload.funcs.decode = h2d_callback;

	if (!pb_decode_delimited(&h2d_stream, usb_comm_MessageH2D_fields, &h2d)) {
		LOG_ERR("Failed decoding h2d message: %s", h2d_stream.errmsg);
		goto next;
	}

	LOG_DBG("req action: %d", h2d.action);
	d2h.action = h2d.action;
	d2h.which_payload = usb_comm_MessageD2H_nop_tag;

	for (size_t i = 0; i < ARRAY_SIZE(handlers); i++) {
		if (handlers[i].action == h2d.action) {
			if (handlers[i].handler(&h2d, &d2h, bytes_field, bytes_field_len)) {
				d2h.which_payload = handlers[i].which_payload;
			}
			break;
		}
	}

	size_t d2h_size;
	pb_get_encoded_size(&d2h_size, usb_comm_MessageD2H_fields, &d2h);
	if (d2h_size > sizeof(usb_tx_buf)) {
		LOG_ERR("The size of response for action %d is %d, exceeds max tx buf size %d",
			h2d.action, d2h_size, sizeof(usb_tx_buf));
	}

	if (!pb_encode_delimited(&d2h_stream, usb_comm_MessageD2H_fields, &d2h)) {
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
