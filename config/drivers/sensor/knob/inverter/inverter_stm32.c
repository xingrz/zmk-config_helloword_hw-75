/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_inverter_stm32

#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>
#include <logging/log.h>
#include <sys/util_macro.h>

#include <soc.h>
#include <stm32_ll_tim.h>

#include <knob/drivers/inverter.h>

LOG_MODULE_REGISTER(inverter_stm32, CONFIG_ZMK_LOG_LEVEL);

static const uint32_t timer_channels[] = {
	TIM_CHANNEL_1,
	TIM_CHANNEL_2,
	TIM_CHANNEL_3,
	TIM_CHANNEL_4,
};

#define TIM_CHANNEL(config, idx) (timer_channels[config->pwm_channels[idx] - 1])

struct inverter_stm32_data {
	TIM_HandleTypeDef th;
};

struct inverter_stm32_config {
	struct gpio_dt_spec enable_gpio;
	TIM_TypeDef *timer;
	int pwm_period;
	int pwm_channels[3];
};

static void inverter_stm32_set_powers(const struct device *dev, float va, float vb, float vc);

static void inverter_stm32_start(const struct device *dev)
{
	struct inverter_stm32_data *data = dev->data;
	const struct inverter_stm32_config *config = dev->config;

	for (int i = 0; i < ARRAY_SIZE(config->pwm_channels); i++) {
		HAL_TIM_PWM_Start(&data->th, TIM_CHANNEL(config, i));
		HAL_TIMEx_PWMN_Start(&data->th, TIM_CHANNEL(config, i));
	}

	if (config->enable_gpio.port != NULL) {
		gpio_pin_set_dt(&config->enable_gpio, true);
		LOG_DBG("Inverter enabled");
	} else {
		LOG_DBG("Enable GPIO not configured, ignored start");
	}
}

static void inverter_stm32_stop(const struct device *dev)
{
	struct inverter_stm32_data *data = dev->data;
	const struct inverter_stm32_config *config = dev->config;

	inverter_stm32_set_powers(dev, 0, 0, 0);

	for (int i = 0; i < ARRAY_SIZE(config->pwm_channels); i++) {
		HAL_TIM_PWM_Stop(&data->th, TIM_CHANNEL(config, i));
		HAL_TIMEx_PWMN_Stop(&data->th, TIM_CHANNEL(config, i));
	}

	if (config->enable_gpio.port != NULL) {
		gpio_pin_set_dt(&config->enable_gpio, false);
		LOG_DBG("Inverter disabled");
	} else {
		LOG_DBG("Enable GPIO not configured, ignored stop");
	}
}

static void inverter_stm32_set_powers(const struct device *dev, float a, float b, float c)
{
	struct inverter_stm32_data *data = dev->data;
	const struct inverter_stm32_config *config = dev->config;

	a = CLAMP(a, 0.0f, 1.0f);
	b = CLAMP(b, 0.0f, 1.0f);
	c = CLAMP(c, 0.0f, 1.0f);

	__HAL_TIM_SET_COMPARE(&data->th, TIM_CHANNEL(config, 0), config->pwm_period * a);
	__HAL_TIM_SET_COMPARE(&data->th, TIM_CHANNEL(config, 1), config->pwm_period * b);
	__HAL_TIM_SET_COMPARE(&data->th, TIM_CHANNEL(config, 2), config->pwm_period * c);
}

static int inverter_stm32_init(const struct device *dev)
{
	struct inverter_stm32_data *data = dev->data;
	const struct inverter_stm32_config *config = dev->config;
	int rc;

	if (config->enable_gpio.port != NULL) {
		if (!device_is_ready(config->enable_gpio.port)) {
			LOG_ERR("%s: enable_gpio not ready", dev->name);
			return -ENODEV;
		}
		rc = gpio_pin_configure_dt(&config->enable_gpio, GPIO_OUTPUT_INACTIVE);
		if (rc < 0) {
			LOG_ERR("%s: couldn't configure enable_gpio (%d)", dev->name, rc);
			return rc;
		}
	}

	data->th.Instance = config->timer;
	data->th.Init.Prescaler = 0;
	data->th.Init.CounterMode = TIM_COUNTERMODE_CENTERALIGNED1;
	data->th.Init.Period = config->pwm_period;
	data->th.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	data->th.Init.RepetitionCounter = 0;
	data->th.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_PWM_Init(&data->th) != HAL_OK) {
		LOG_ERR("%s: HAL_TIM_PWM_Init failed", dev->name);
		return -EIO;
	}

	TIM_MasterConfigTypeDef master_config = { 0 };
	master_config.MasterOutputTrigger = TIM_TRGO_OC4REF;
	master_config.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&data->th, &master_config) != HAL_OK) {
		LOG_ERR("%s: HAL_TIMEx_MasterConfigSynchronization failed", dev->name);
		return -EIO;
	}

	TIM_OC_InitTypeDef oc = { 0 };
	oc.OCMode = TIM_OCMODE_PWM1;
	oc.Pulse = 0;
	oc.OCPolarity = TIM_OCPOLARITY_HIGH;
	oc.OCNPolarity = TIM_OCNPOLARITY_HIGH;
	oc.OCFastMode = TIM_OCFAST_DISABLE;
	oc.OCIdleState = TIM_OCIDLESTATE_RESET;
	oc.OCNIdleState = TIM_OCNIDLESTATE_RESET;
	for (int i = 0; i < ARRAY_SIZE(config->pwm_channels); i++) {
		if (HAL_TIM_PWM_ConfigChannel(&data->th, &oc, TIM_CHANNEL(config, i)) != HAL_OK) {
			LOG_ERR("%s: HAL_TIM_PWM_ConfigChannel for channel %d failed", dev->name,
				config->pwm_channels[i]);
			return -EIO;
		}
	}

	TIM_BreakDeadTimeConfigTypeDef bdt = { 0 };
	bdt.OffStateRunMode = TIM_OSSR_DISABLE;
	bdt.OffStateIDLEMode = TIM_OSSI_DISABLE;
	bdt.LockLevel = TIM_LOCKLEVEL_OFF;
	bdt.DeadTime = 0;
	bdt.BreakState = TIM_BREAK_DISABLE;
	bdt.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
	bdt.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
	if (HAL_TIMEx_ConfigBreakDeadTime(&data->th, &bdt) != HAL_OK) {
		LOG_ERR("%s: HAL_TIMEx_ConfigBreakDeadTime failed", dev->name);
		return -EIO;
	}

	return 0;
}

static const struct inverter_driver_api inverter_stm32_driver_api = {
	.start = inverter_stm32_start,
	.stop = inverter_stm32_stop,
	.set_powers = inverter_stm32_set_powers,
};

#define INVERTER_STM32_INST(n)                                                                     \
	struct inverter_stm32_data inverter_stm32_data_##n;                                        \
                                                                                                   \
	static const struct inverter_stm32_config inverter_stm32_config_##n = {                    \
		.enable_gpio = GPIO_DT_SPEC_INST_GET_OR(n, enable_gpios, { 0 }),                   \
		.timer = (TIM_TypeDef *)DT_REG_ADDR(DT_INST_PARENT(n)),                            \
		.pwm_period = DT_INST_PROP(n, pwm_period),                                         \
		.pwm_channels = DT_INST_PROP(n, pwm_channels),                                     \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, inverter_stm32_init, NULL, &inverter_stm32_data_##n,              \
			      &inverter_stm32_config_##n, POST_KERNEL,                             \
			      CONFIG_KNOB_DRIVER_INIT_PRIORITY, &inverter_stm32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(INVERTER_STM32_INST)
