/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zephyr/storage/flash_map.h>
#include <zephyr/fs/nvs.h>

#define STORAGE_PARTITION FIXED_PARTITION_ID(storage_partition)

static int storage_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	int ret;

	const struct flash_area *fa;
	ret = flash_area_open(STORAGE_PARTITION, &fa);
	if (ret != 0) {
		LOG_ERR("Failed to open storage flash area: %d", ret);
		goto err_fa;
	}

	struct flash_sector sectors[8];
	uint32_t sector_cnt = ARRAY_SIZE(sectors);
	ret = flash_area_get_sectors(STORAGE_PARTITION, &sector_cnt, sectors);
	if (ret != 0) {
		LOG_ERR("Failed to get sector info of flash area: %d", ret);
		goto err_nvs;
	}

	LOG_DBG("sector size: %d", sectors[0].fs_size);
	LOG_DBG("sector count: %d", sector_cnt);

	struct nvs_fs nvs = {
		.offset = fa->fa_off,
		.sector_size = sectors[0].fs_size,
		.sector_count = sector_cnt,
		.flash_device = fa->fa_dev,
	};

	ret = nvs_mount(&nvs);
	if (ret == 0) {
		LOG_DBG("NVS is ready to use");
		goto err_nvs;
	}
	if (ret != -EDEADLK) {
		LOG_ERR("Failed to init NVS: %d", ret);
		goto err_nvs;
	}

	LOG_DBG("Start to erase dirty storage");

	ret = flash_area_erase(fa, 0, fa->fa_size);
	if (ret != 0) {
		LOG_ERR("Failed erasing storage: %d", ret);
		goto err_nvs;
	}

	LOG_DBG("Erase done");

err_nvs:
	flash_area_close(fa);
err_fa:
	return ret;
}

// should be less than CONFIG_APPLICATION_INIT_PRIORITY (90)
#define CONFIG_STORAGE_ERASE_INIT_PRIORITY 80

SYS_INIT(storage_init, APPLICATION, CONFIG_STORAGE_ERASE_INIT_PRIORITY);
