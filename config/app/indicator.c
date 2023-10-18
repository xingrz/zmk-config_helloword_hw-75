/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/settings/settings.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zephyr/drivers/led_strip_remap.h>

#include <zmk/workqueue.h>
#include <zmk/event_manager.h>
#include <zmk/events/activity_state_changed.h>

#include <app/indicator.h>

#define STRIP_CHOSEN          DT_CHOSEN(zmk_underglow)
#define STRIP_INDICATOR_LABEL "STATUS"

#define RGB(R, G, B)  ((struct led_rgb){.r = (R), .g = (G), .b = (B)})
#define BRI(rgb, bri) RGB(rgb.r *bri / 255, rgb.g * bri / 255, rgb.b * bri / 255)

#define RED   (RGB(0xFF, 0x00, 0x00))
#define GREEN (RGB(0x00, 0xFF, 0x00))

static const struct device *led_strip;

static struct indicator_settings settings = {
	.enable = true,
	.brightness_active = CONFIG_HW75_INDICATOR_BRIGHTNESS_ACTIVE,
	.brightness_inactive = CONFIG_HW75_INDICATOR_BRIGHTNESS_INACTIVE,
};

static struct k_mutex lock;

static struct led_rgb current;
static bool active = true;

static uint32_t state = 0;

static inline struct led_rgb apply_brightness(struct led_rgb color, uint8_t bri)
{
	return RGB(color.r * bri / 255, color.g * bri / 255, color.b * bri / 255);
}

static void indicator_update(struct k_work *work)
{
	if (!settings.enable) {
		unsigned int key = irq_lock();
		led_strip_remap_clear(led_strip, STRIP_INDICATOR_LABEL);
		irq_unlock(key);
		return;
	}

	current = state ? RED : GREEN;

	uint8_t bri = active ? settings.brightness_active : settings.brightness_inactive;
	struct led_rgb color = BRI(current, bri);

	LOG_DBG("Update indicator, color: %02X%02X%02X, brightness: %d -> %02X%02X%02X", current.r,
		current.g, current.b, bri, color.r, color.g, color.b);

	unsigned int key = irq_lock();
	led_strip_remap_set(led_strip, STRIP_INDICATOR_LABEL, &color);
	irq_unlock(key);
}

K_WORK_DEFINE(indicator_update_work, indicator_update);

static inline void post_indicator_update(void)
{
	k_work_submit_to_queue(zmk_workqueue_lowprio_work_q(), &indicator_update_work);
}

uint32_t indicator_set_bits(uint32_t bits)
{
	k_mutex_lock(&lock, K_FOREVER);
	state |= bits;
	k_mutex_unlock(&lock);
	post_indicator_update();
	return state;
}

uint32_t indicator_clear_bits(uint32_t bits)
{
	k_mutex_lock(&lock, K_FOREVER);
	state &= ~bits;
	k_mutex_unlock(&lock);
	post_indicator_update();
	return state;
}

#ifdef CONFIG_SETTINGS
static int indicator_settings_load_cb(const char *name, size_t len, settings_read_cb read_cb,
				      void *cb_arg, void *param)
{
	const char *next;
	int ret;

	if (settings_name_steq(name, "settings", &next) && !next) {
		if (len != sizeof(settings)) {
			return -EINVAL;
		}

		ret = read_cb(cb_arg, &settings, sizeof(settings));
		if (ret >= 0) {
			return 0;
		}

		LOG_DBG("Loaded indicator settings");

		return ret;
	}

	return -ENOENT;
}

static void indicator_save_settings_work(struct k_work *work)
{
	ARG_UNUSED(work);
	int ret = settings_save_one("app/indicator/settings", &settings, sizeof(settings));
	if (ret != 0) {
		LOG_ERR("Failed saving settings: %d", ret);
	} else {
		LOG_DBG("Saved indicator settings");
	}
}

K_WORK_DELAYABLE_DEFINE(indicator_save_work, indicator_save_settings_work);
#endif

int indicator_save_settings(void)
{
#ifdef CONFIG_SETTINGS
	int ret =
		k_work_reschedule(&indicator_save_work, K_MSEC(CONFIG_ZMK_SETTINGS_SAVE_DEBOUNCE));
	return MIN(ret, 0);
#else
	return 0;
#endif
}

static void indicator_clear_preview(struct k_work *work)
{
	ARG_UNUSED(work);
	post_indicator_update();
}

K_WORK_DELAYABLE_DEFINE(indicator_clear_preview_work, indicator_clear_preview);

static void indicator_preview_brightness(uint8_t brightness)
{
	struct led_rgb color = BRI(current, brightness);

	LOG_DBG("Preview indicator, color: %02X%02X%02X, brightness: %d -> %02X%02X%02X", current.r,
		current.g, current.b, brightness, color.r, color.g, color.b);

	unsigned int key = irq_lock();
	led_strip_remap_set(led_strip, "STATUS", &color);
	irq_unlock(key);

	k_work_reschedule(&indicator_clear_preview_work, K_MSEC(2000));
}

void indicator_set_enable(bool enable)
{
	settings.enable = enable;
	if (!enable) {
		settings.brightness_active = CONFIG_HW75_INDICATOR_BRIGHTNESS_ACTIVE;
		settings.brightness_inactive = CONFIG_HW75_INDICATOR_BRIGHTNESS_INACTIVE;
	}
	indicator_save_settings();
	post_indicator_update();
}

void indicator_set_brightness_active(uint8_t brightness)
{
	settings.brightness_active = brightness;
	indicator_save_settings();
	indicator_preview_brightness(brightness);
}

void indicator_set_brightness_inactive(uint8_t brightness)
{
	settings.brightness_inactive = brightness;
	indicator_save_settings();
	indicator_preview_brightness(brightness);
}

const struct indicator_settings *indicator_get_settings(void)
{
	return &settings;
}

static int indicator_event_listener(const zmk_event_t *eh)
{
	struct zmk_activity_state_changed *activity_ev;

	if ((activity_ev = as_zmk_activity_state_changed(eh)) != NULL) {
		active = activity_ev->state == ZMK_ACTIVITY_ACTIVE;
		post_indicator_update();
		return 0;
	}

	return -ENOTSUP;
}

static int indicator_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	int ret;

	led_strip = DEVICE_DT_GET(STRIP_CHOSEN);

#ifdef CONFIG_SETTINGS
	ret = settings_subsys_init();
	if (ret) {
		LOG_ERR("Failed to initializing settings subsys: %d", ret);
	}

	ret = settings_load_subtree_direct("app/indicator", indicator_settings_load_cb, NULL);
	if (ret) {
		LOG_ERR("Failed to load indicator settings: %d", ret);
	}
#endif

	k_mutex_init(&lock);
	k_work_submit_to_queue(zmk_workqueue_lowprio_work_q(), &indicator_update_work);

	return 0;
}

ZMK_LISTENER(indicator, indicator_event_listener);
ZMK_SUBSCRIPTION(indicator, zmk_activity_state_changed);

SYS_INIT(indicator_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
