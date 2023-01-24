/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#include <kernel.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(usb_comm, CONFIG_HW75_USB_COMM_LOG_LEVEL);

#include <sys/byteorder.h>
#include <usb/usb_device.h>
#include <usb/bos.h>

#include "usb_comm_webusb.h"

#define WBVAL(x) (x & 0xFF), ((x >> 8) & 0xFF)

#define WINUSB_VENDOR_CODE 0x01
#define WINUSB_REQUEST_GET_DESCRIPTOR_SET 0x07

#define WINUSB_DESCRIPTOR_SET_HEADER_SIZE 10
#define WINUSB_FUNCTION_SUBSET_HEADER_SIZE 8
#define WINUSB_FEATURE_COMPATIBLE_ID_SIZE 20
#define WINUSB_FEATURE_REG_PROPERTY_SIZE 132

#define WINUSB_SET_HEADER_DESCRIPTOR_TYPE 0x00
#define WINUSB_SUBSET_HEADER_CONFIGURATION_TYPE 0x01
#define WINUSB_SUBSET_HEADER_FUNCTION_TYPE 0x02
#define WINUSB_FEATURE_COMPATIBLE_ID_TYPE 0x03
#define WINUSB_FEATURE_REG_PROPERTY_TYPE 0x04

#define WINUSB_PROP_DATA_TYPE_REG_MULTI_SZ 0x07

static const uint8_t msos2_descriptor[] = {
	/* Microsoft OS 2.0 descriptor set header */
	WBVAL(WINUSB_DESCRIPTOR_SET_HEADER_SIZE), // wLength
	WBVAL(WINUSB_SET_HEADER_DESCRIPTOR_TYPE), // wDescriptorType
	0x00, 0x00, 0x03, 0x06, // Windows version (8.1) (0x06030000)
	WBVAL(178), // wTotalLength

	/* Microsoft OS 2.0 configuration subset header */
	WBVAL(WINUSB_FUNCTION_SUBSET_HEADER_SIZE), // wLength
	WBVAL(WINUSB_SUBSET_HEADER_CONFIGURATION_TYPE), // wDescriptorType
	0, // bConfigurationValue
	0, // bReserved
	WBVAL(168), // wSubsetLength

	/* Microsoft OS 2.0 function subset header */
	WBVAL(WINUSB_FUNCTION_SUBSET_HEADER_SIZE), // wLength
	WBVAL(WINUSB_SUBSET_HEADER_FUNCTION_TYPE), // wDescriptorType
	0, // bFirstInterface
	0, // bReserved
	WBVAL(160), // wSubsetLength

	/* Microsoft OS 2.0 compatible ID descriptor */
	WBVAL(WINUSB_FEATURE_COMPATIBLE_ID_SIZE), // wLength
	WBVAL(WINUSB_FEATURE_COMPATIBLE_ID_TYPE), // wDescriptorType
	'W', 'I', 'N', 'U', 'S', 'B', 0, 0, // CompatibleId
	0, 0, 0, 0, 0, 0, 0, 0, // SubCompatibleId

	/* Microsoft OS 2.0 registry property descriptor */
	WBVAL(WINUSB_FEATURE_REG_PROPERTY_SIZE), // wLength
	WBVAL(WINUSB_FEATURE_REG_PROPERTY_TYPE), // wDescriptorType
	WBVAL(WINUSB_PROP_DATA_TYPE_REG_MULTI_SZ), // wPropertyDataType
	WBVAL(42), // wPropertyNameLength
	/* "DeviceInterfaceGUIDs\0" */
	'D', 0, 'e', 0, 'v', 0, 'i', 0, 'c', 0, 'e', 0, //
	'I', 0, 'n', 0, 't', 0, 'e', 0, 'r', 0, 'f', 0, 'a', 0, 'c', 0, 'e', 0, //
	'G', 0, 'U', 0, 'I', 0, 'D', 0, 's', 0, 0x00, 0, //
	WBVAL(80), // wPropertyDataLength
	/* "{52267e5b-818d-43c2-a17d-7b185f4d9afe}\0\0" */
	'{', 0, //
	'5', 0, '2', 0, '2', 0, '6', 0, '7', 0, 'E', 0, '5', 0, 'B', 0, '-', 0, //
	'8', 0, '1', 0, '8', 0, 'D', 0, '-', 0, //
	'4', 0, '3', 0, 'C', 0, '2', 0, '-', 0, //
	'A', 0, '1', 0, '7', 0, 'D', 0, '-', 0, //
	'7', 0, 'B', 0, '1', 0, '8', 0, //
	'5', 0, 'F', 0, '4', 0, 'D', 0, //
	'9', 0, 'A', 0, 'F', 0, 'E', 0, //
	'}', 0, 0x00, 0, 0x00, 0, //
};

USB_DEVICE_BOS_DESC_DEFINE_CAP struct usb_bos_msosv2_desc {
	struct usb_bos_platform_descriptor platform;
	struct usb_bos_capability_msos cap;
} __packed bos_cap_msosv2 = {
	.platform = {
		.bLength = sizeof(struct usb_bos_platform_descriptor)
			+ sizeof(struct usb_bos_capability_msos),
		.bDescriptorType = USB_DESC_DEVICE_CAPABILITY,
		.bDevCapabilityType = USB_BOS_CAPABILITY_PLATFORM,
		.bReserved = 0,
		/**
		 * MS OS 2.0 Platform Capability ID
		 * D8DD60DF-4589-4CC7-9CD2-659D9E648A9F
		 */
		.PlatformCapabilityUUID = {
			0xDF, 0x60, 0xDD, 0xD8,
			0x89, 0x45,
			0xC7, 0x4C,
			0x9C, 0xD2,
			0x65, 0x9D, 0x9E, 0x64, 0x8A, 0x9F,
		},
	},
	.cap = {
		/* Windows version (8.1) (0x06030000) */
		.dwWindowsVersion = sys_cpu_to_le32(0x06030000),
		.wMSOSDescriptorSetTotalLength =
			sys_cpu_to_le16(sizeof(msos2_descriptor)),
		.bMS_VendorCode = WINUSB_VENDOR_CODE,
		.bAltEnumCode = 0x00,
	},
};

#define WEBUSB_VENDOR_CODE 0x02
#define WEBUSB_REQUEST_GET_URL 0x02

static const uint8_t webusb_origin_url[] = {
	/* URL Descriptor */
	0x0f, // bLength
	0x03, // bDescriptorType (url)
	0x01, 'h', 'w', '.', 'x', 'i', 'n', 'g', 'r', 'z', '.', 'm', 'e'
};

USB_DEVICE_BOS_DESC_DEFINE_CAP struct usb_bos_webusb_desc {
	struct usb_bos_platform_descriptor platform;
	struct usb_bos_capability_webusb cap;
} __packed bos_cap_webusb = {
	.platform = {
		.bLength = sizeof(struct usb_bos_platform_descriptor)
			+ sizeof(struct usb_bos_capability_webusb),
		.bDescriptorType = USB_DESC_DEVICE_CAPABILITY,
		.bDevCapabilityType = USB_BOS_CAPABILITY_PLATFORM,
		.bReserved = 0,
		/**
		 * WebUSB Platform Capability UUID
		 * 3408b638-09a9-47a0-8bfd-a0768815b665
		 */
		.PlatformCapabilityUUID = {
			0x38, 0xB6, 0x08, 0x34,
			0xA9, 0x09,
			0xA0, 0x47,
			0x8B, 0xFD,
			0xA0, 0x76, 0x88, 0x15, 0xB6, 0x65,
		},
	},
	.cap = {
		.bcdVersion = sys_cpu_to_le16(0x0100),
		.bVendorCode = WEBUSB_VENDOR_CODE,
		.iLandingPage = 1,
	}
};

int usb_comm_custom_handler(struct usb_setup_packet *setup, int32_t *len, uint8_t **data)
{
	return -EINVAL;
}

int usb_comm_vendor_handler(struct usb_setup_packet *setup, int32_t *len, uint8_t **data)
{
	if (usb_reqtype_is_to_device(setup)) {
		return -ENOTSUP;
	}

	if (setup->bRequest == WEBUSB_VENDOR_CODE && setup->wIndex == WEBUSB_REQUEST_GET_URL) {
		*data = (uint8_t *)(&webusb_origin_url);
		*len = sizeof(webusb_origin_url);

		LOG_DBG("Get webusb_origin_url");

		return 0;
	} else if (setup->bRequest == WINUSB_VENDOR_CODE &&
		   setup->wIndex == WINUSB_REQUEST_GET_DESCRIPTOR_SET) {
		*data = (uint8_t *)(&msos2_descriptor);
		*len = sizeof(msos2_descriptor);

		LOG_DBG("Get MS OS Descriptors v2");

		return 0;
	}

	return -ENOTSUP;
}

static int usb_comm_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	usb_bos_register_cap((void *)&bos_cap_msosv2);
	usb_bos_register_cap((void *)&bos_cap_webusb);

	return 0;
}

SYS_INIT(usb_comm_init, APPLICATION, CONFIG_ZMK_USB_INIT_PRIORITY);
