/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/pm/device_runtime.h>
#include "my_lbs.h"
#include <zephyr/device.h>
#include <nrfx_saadc.h>
#include <stdbool.h>
#include "solar.h"
#include "pwm_control.h"

#define CHARGE_NODE DT_ALIAS(led1)
#define LED_WARM_NODE DT_ALIAS(led0)
#define LED_COOL_NODE DT_ALIAS(led3)

#define ADC_REF_VOLTAGE_IN_MILLIVOLTS   600
#define ADC_PRE_SCALING_COMPENSATION    6
#define ADC_RES_10BIT                   1024
#define ADC_RESULT_IN_MILLI_VOLTS(ADC_VALUE) \
        ((((ADC_VALUE) * ADC_REF_VOLTAGE_IN_MILLIVOLTS) / ADC_RES_10BIT) * ADC_PRE_SCALING_COMPENSATION)

static const struct gpio_dt_spec chargePin  = GPIO_DT_SPEC_GET(CHARGE_NODE, gpios);
static const struct gpio_dt_spec ledWarmPin = GPIO_DT_SPEC_GET(LED_WARM_NODE, gpios);
static const struct gpio_dt_spec ledCoolPin = GPIO_DT_SPEC_GET(LED_COOL_NODE, gpios);

static struct bt_le_adv_param *adv_param = BT_LE_ADV_PARAM(
	(BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_USE_IDENTITY),
	3200, /* Min Advertising Interval 2000ms (3200*0.625ms) */
	3201, /* Max Advertising Interval 2000.625ms (3201*0.625ms) */
	NULL);

LOG_MODULE_REGISTER(xlite, LOG_LEVEL_DBG);

#define DEVICE_NAME     CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

/* Manufacturer-specific payload: [company_lo, company_hi, warm, cool, batt_lo, batt_hi, solar_lo, solar_hi]
 * Company ID 0xFFFF = not registered (development use only) */
static uint8_t mfr_data[8] = { 0xFF, 0xFF, 0, 0, 0, 0, 0, 0 };

static struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
	BT_DATA(BT_DATA_MANUFACTURER_DATA, mfr_data, sizeof(mfr_data)),
};

static const struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_XLITE_VAL),
};

static void update_adv(uint8_t warm, uint8_t cool, uint32_t batt_mv, int32_t solar_mv)
{
	mfr_data[2] = warm;
	mfr_data[3] = cool;
	mfr_data[4] = (uint8_t)(batt_mv & 0xFF);
	mfr_data[5] = (uint8_t)((batt_mv >> 8) & 0xFF);
	mfr_data[6] = (uint8_t)(solar_mv & 0xFF);
	mfr_data[7] = (uint8_t)((solar_mv >> 8) & 0xFF);
	bt_le_adv_update_data(ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
}

static void app_brightness_cb(uint8_t led_brightness)
{
	pwmSetWarmLightDuty((uint32_t)led_brightness);
}

static void app_color_cb(uint8_t led_color)
{
	pwmSetCoolLightDuty((uint32_t)led_color);
}

static struct my_xlite_cb app_callbacks = {
	.brightness_cb = app_brightness_cb,
	.color_cb      = app_color_cb,
};

/* Request fast connection: 15ms interval, no latency, 4s timeout */
static const struct bt_le_conn_param fast_conn_params = {
	.interval_min = 12,  /* 12 * 1.25ms = 15ms */
	.interval_max = 12,
	.latency      = 0,
	.timeout      = 400, /* 400 * 10ms = 4s */
};

static volatile bool ble_connected = false;

static void on_connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		return;
	}
	ble_connected = true;
	bt_conn_le_param_update(conn, &fast_conn_params);
}

static void on_disconnected(struct bt_conn *conn, uint8_t reason)
{
	ble_connected = false;
}

struct bt_conn_cb connection_callbacks = {
	.connected    = on_connected,
	.disconnected = on_disconnected,
};

int main(void)
{
	int err;

	/* Drive LED enable lines LOW immediately - floating inputs cause
	 * N-channel FETs to turn on, pulling current through R8/R9 pull-ups */
	gpio_pin_configure_dt(&ledWarmPin, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&ledCoolPin, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&chargePin,  GPIO_OUTPUT_INACTIVE);

	init_pwm();

	err = bt_enable(NULL);
	if (err) {
		return err;
	}
	bt_conn_cb_register(&connection_callbacks);

	err = my_xlite_init(&app_callbacks);
	if (err) {
		return err;
	}

	err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err) {
		return err;
	}

	/* Suspend PWM so HFCLK can be released during sleep */
	turnLightOff();

	for (;;) {
		/* Disable charge FET and wait for voltage to settle */
		gpio_pin_set_dt(&chargePin, 0);
		k_sleep(K_MSEC(100));

		int16_t solarValue = getSolarVolts();
		if (solarValue < 1) {
			solarValue = 2;
		}
		int32_t solar_lvl_in_milli_volts = ADC_RESULT_IN_MILLI_VOLTS(solarValue);
		solar_lvl_in_milli_volts = (int32_t)(solar_lvl_in_milli_volts * 2.43f);

		uint32_t batteryValue = getBatteryVolts();

		if (solar_lvl_in_milli_volts <= getSolarThreshold()) {
			pwmSetCoolLightDuty(getCoolLightBrightness());
			pwmSetWarmLightDuty(getWarmLightBrightness());
		} else {
			suspendLight();
			if (solar_lvl_in_milli_volts > batteryValue && batteryValue < 3320) {
				gpio_pin_set_dt(&chargePin, 1);
			}
		}

		/* Store solar for BLE characteristic reads and update advertisement */
		setSolarValue(solar_lvl_in_milli_volts);
		update_adv((uint8_t)getWarmLightBrightness(),
			   (uint8_t)getCoolLightBrightness(),
			   batteryValue,
			   solar_lvl_in_milli_volts);

		/* 1s loop when phone is connected for live readings, 10s otherwise */
		k_sleep(ble_connected ? K_SECONDS(1) : K_SECONDS(10));
	}
}
