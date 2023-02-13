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

#include "usb_comm_hid.h"
#include "usb_comm.pb.h"

#include "handler/handler.h"

static struct k_sem usb_comm_sem;

static K_THREAD_STACK_DEFINE(usb_comm_thread_stack, CONFIG_HW75_USB_COMM_THREAD_STACK_SIZE);
static struct k_thread usb_comm_thread;

static uint32_t usb_rx_idx, usb_rx_len;
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

#ifdef CONFIG_HW75_USB_COMM_FEATURE_KNOB
	{ usb_comm_Action_MOTOR_GET_STATE, usb_comm_MessageD2H_motor_state_tag,
	  handle_motor_get_state },
	{ usb_comm_Action_KNOB_GET_CONFIG, usb_comm_MessageD2H_knob_config_tag,
	  handle_knob_get_config },
	{ usb_comm_Action_KNOB_SET_CONFIG, usb_comm_MessageD2H_knob_config_tag,
	  handle_knob_set_config },
#endif // CONFIG_HW75_USB_COMM_FEATURE_KNOB

#ifdef CONFIG_HW75_USB_COMM_FEATURE_RGB
	{ usb_comm_Action_RGB_CONTROL, usb_comm_MessageD2H_rgb_state_tag, handle_rgb_control },
	{ usb_comm_Action_RGB_GET_STATE, usb_comm_MessageD2H_rgb_state_tag, handle_rgb_get_state },
	{ usb_comm_Action_RGB_SET_STATE, usb_comm_MessageD2H_rgb_state_tag, handle_rgb_set_state },
	{ usb_comm_Action_RGB_GET_INDICATOR, usb_comm_MessageD2H_rgb_indicator_tag,
	  handle_rgb_get_indicator },
	{ usb_comm_Action_RGB_SET_INDICATOR, usb_comm_MessageD2H_rgb_indicator_tag,
	  handle_rgb_set_indicator },
#endif // CONFIG_HW75_USB_COMM_FEATURE_RGB

#ifdef CONFIG_HW75_USB_COMM_FEATURE_EINK
	{ usb_comm_Action_EINK_SET_IMAGE, usb_comm_MessageD2H_eink_image_tag,
	  handle_eink_set_image },
#endif // CONFIG_HW75_USB_COMM_FEATURE_EINK
};

#if CONFIG_HW75_USB_COMM_MAX_BYTES_FIELD_SIZE
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
#endif

#if CONFIG_HW75_USB_COMM_MAX_BYTES_FIELD_SIZE
static bool h2d_callback(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
	if (field->tag == usb_comm_MessageH2D_eink_image_tag) {
		usb_comm_EinkImage *eink_image = field->pData;
		eink_image->bits.funcs.decode = read_bytes_field;
	}
	return true;
}
#endif

static void usb_comm_handle_message()
{
	LOG_DBG("message size %u", usb_rx_len);
	LOG_HEXDUMP_DBG(usb_rx_buf, MIN(usb_rx_len, 64), "message data");

	pb_istream_t h2d_stream = pb_istream_from_buffer(usb_rx_buf, usb_rx_len);
	pb_ostream_t d2h_stream = pb_ostream_from_buffer(usb_tx_buf, sizeof(usb_tx_buf));

	usb_comm_MessageH2D h2d = usb_comm_MessageH2D_init_zero;
	usb_comm_MessageD2H d2h = usb_comm_MessageD2H_init_zero;

#if CONFIG_HW75_USB_COMM_MAX_BYTES_FIELD_SIZE
	h2d.cb_payload.funcs.decode = h2d_callback;
#endif

	if (!pb_decode_delimited(&h2d_stream, usb_comm_MessageH2D_fields, &h2d)) {
		LOG_ERR("Failed decoding h2d message: %s", h2d_stream.errmsg);
		return;
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
		return;
	}

	usb_comm_hid_send(usb_tx_buf, d2h_stream.bytes_written);
}

static void usb_comm_handle_packet(uint8_t *data, uint32_t len)
{
	if (usb_rx_idx + len > sizeof(usb_rx_buf)) {
		LOG_ERR("RX buffer overflows, index: %d, received: %d", usb_rx_idx, len);
		usb_rx_idx = 0;
		return;
	}

	if (data[0] + 1 > len) {
		LOG_ERR("Invalid packet header: %d, len: %d", data[0], len);
		return;
	}

	memcpy(usb_rx_buf + usb_rx_idx, data + 1, data[0]);
	usb_rx_idx += data[0];

	if (data[0] + 1 < len) {
		usb_rx_len = usb_rx_idx;
		usb_rx_idx = 0;
		k_sem_give(&usb_comm_sem);
	}
}

static void usb_comm_thread_entry(void *p1, void *p2, void *p3)
{
	usb_comm_hid_init(usb_comm_handle_packet);
	while (true) {
		k_sem_take(&usb_comm_sem, K_FOREVER);
		usb_comm_handle_message();
	}
}

static int usb_comm_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	k_sem_init(&usb_comm_sem, 0, 1);

	k_thread_create(&usb_comm_thread, usb_comm_thread_stack,
			CONFIG_HW75_USB_COMM_THREAD_STACK_SIZE, usb_comm_thread_entry, NULL, NULL,
			NULL, K_PRIO_COOP(CONFIG_HW75_USB_COMM_THREAD_PRIORITY), 0, K_NO_WAIT);

	return 0;
}

SYS_INIT(usb_comm_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
