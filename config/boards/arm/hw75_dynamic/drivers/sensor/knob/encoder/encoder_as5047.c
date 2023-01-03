/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT ams_as5047

#include <device.h>
#include <devicetree.h>
#include <drivers/spi.h>
#include <logging/log.h>
#include <sys/byteorder.h>
#include <sys/util_macro.h>

#include <knob/math.h>
#include <knob/drivers/encoder.h>

LOG_MODULE_REGISTER(as5047, CONFIG_ZMK_LOG_LEVEL);

#define ADDR_NOP 0x0000
#define ADDR_ERRFL 0x0001
#define ADDR_PROG 0x0003
#define ADDR_DIAAGC 0x3FFC
#define ADDR_MAG 0x3FFD
#define ADDR_ANGLEUNC 0x3FFE
#define ADDR_ANGLECOM 0x3FFF

#define RW_WRITE 0
#define RW_READ 1

struct as5047_config {
	struct spi_dt_spec bus;
	uint32_t init_delay_us;
};

static inline uint8_t as5047_calc_parc(uint16_t command)
{
	uint8_t parc = 0;
	for (uint8_t i = 0; i < 16; i++) {
		if (command & 0x1)
			parc++;
		command >>= 1;
	}
	return parc & 0x1;
}

static int as5047_send_command(const struct device *dev, uint16_t addr, uint8_t rw, uint16_t *val)
{
	const struct as5047_config *config = dev->config;

	int ret;
	uint16_t req, res = 0;

	req = (addr & BIT_MASK(14)) | ((rw & BIT_MASK(1)) << 14);
	req |= as5047_calc_parc(req) << 15;

	req = sys_cpu_to_be16(req);

	const struct spi_buf tx_buf = {
		.buf = &req,
		.len = sizeof(req),
	};
	const struct spi_buf_set tx_bufs = {
		.buffers = &tx_buf,
		.count = 1U,
	};

	const struct spi_buf rx_buf = {
		.buf = &res,
		.len = sizeof(res),
	};
	const struct spi_buf_set rx_bufs = {
		.buffers = &rx_buf,
		.count = 1U,
	};

	ret = spi_transceive_dt(&config->bus, &tx_bufs, &rx_bufs);
	if (ret != 0) {
		return ret;
	}

	res = sys_be16_to_cpu(res);

	if ((res >> 14) & BIT_MASK(1)) {
		return EIO;
	}

	if ((res >> 15) != as5047_calc_parc(res & BIT_MASK(15))) {
		return EIO;
	}

	*val = res & BIT_MASK(14);

	return ret;
}

static float as5047_get_radian(const struct device *dev)
{
	uint16_t val = 0;
	as5047_send_command(dev, ADDR_ANGLECOM, RW_READ, &val);

	return (float)val * PI2 / (float)BIT(14);
}

static int as5047_init(const struct device *dev)
{
	const struct as5047_config *config = dev->config;

	if (!device_is_ready(config->bus.bus)) {
		LOG_ERR("SPI bus is not ready: %s", config->bus.bus->name);
		return -ENODEV;
	}

	if (config->init_delay_us > 0) {
		k_usleep(config->init_delay_us);
	}

	return 0;
}

static const struct encoder_driver_api as5047_driver_api = {
	.get_radian = as5047_get_radian,
};

#define AS5047_INST(n)                                                                             \
	static const struct as5047_config as5047_config_##n = {                                    \
		.bus = SPI_DT_SPEC_INST_GET(n,                                                     \
					    SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB |                \
						    SPI_WORD_SET(8) | SPI_MODE_CPHA,               \
					    DT_INST_PROP_OR(n, cs_delay_us, 350)),                 \
		.init_delay_us = DT_INST_PROP_OR(n, init_delay_us, 0),                             \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, as5047_init, NULL, NULL, &as5047_config_##n, POST_KERNEL,         \
			      CONFIG_KNOB_DRIVER_INIT_PRIORITY, &as5047_driver_api);

DT_INST_FOREACH_STATUS_OKAY(AS5047_INST)
