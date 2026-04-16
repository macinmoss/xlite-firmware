/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief LED Button Service (LBS) sample
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>
#include <app_version.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include "my_lbs.h"
#include "nrfx_pwm.h"
#include "pwm_control.h"
#include "solar.h"


#include <zephyr/logging/log.h>


//LOG_MODULE_DECLARE(Lesson4_Exercise1);
LOG_MODULE_REGISTER(LBS, LOG_LEVEL_DBG);

static struct my_xlite_cb xlite_cb;
static uint8_t brightness = 0;
static uint8_t color = 0;
static uint16_t vbatt = 0;
static uint8_t solar_mode = 1; /* 0=Early/1600mV, 1=Normal/1700mV, 2=Late/1800mV */
static int32_t solar_mv = 0;

int32_t getSolarThreshold(void)
{
	switch (solar_mode) {
	case 0:  return 1600;
	case 2:  return 1800;
	default: return 1700;
	}
}

void setSolarValue(int32_t mv)
{
	solar_mv = mv;
}

static ssize_t read_xlite_solar(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &solar_mv, sizeof(solar_mv));
}


/* STEP 6 - Implement the write callback function of the LED characteristic */

static ssize_t write_xlite_brightness(struct bt_conn *conn,
			 const struct bt_gatt_attr *attr,
			 const void *buf,
			 uint16_t len, uint16_t offset, uint8_t flags)
{
	LOG_DBG("Attribute write, handle: %u, conn: %p", attr->handle,
		(void *)conn);

	if (len != 1U) {
		LOG_DBG("Write led: Incorrect data length");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	if (offset != 0) {
		LOG_DBG("Write led: Incorrect data offset");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (xlite_cb.brightness_cb) {
		//Read the received value 
		uint8_t val = *((uint8_t *)buf);
		//TODO: set the Brightness
		xlite_cb.brightness_cb(val);
	}

	return len;
}

static ssize_t read_xlite_brightness(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			   uint16_t len, uint16_t offset)
{
		brightness = getWarmLightBrightness();
		return bt_gatt_attr_read(conn, attr, buf, len, offset, &brightness, sizeof(brightness));
}

static ssize_t write_xlite_color(struct bt_conn *conn,
			 const struct bt_gatt_attr *attr,
			 const void *buf,
			 uint16_t len, uint16_t offset, uint8_t flags)
{
	LOG_DBG("Attribute write, handle: %u, conn: %p", attr->handle,
		(void *)conn);

	if (len != 1U) {
		LOG_DBG("Write led: Incorrect data length");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	if (offset != 0) {
		LOG_DBG("Write led: Incorrect data offset");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (xlite_cb.color_cb) {
		//Read the received value 
		uint8_t val = *((uint8_t *)buf);
		//TODO: set the color 
		xlite_cb.color_cb(val);
	}

	return len;
}

static ssize_t read_xlite_color(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			   uint16_t len, uint16_t offset)
{
	
		 color = getCoolLightBrightness();
		
		return bt_gatt_attr_read(conn, attr, buf, len, offset, &color, sizeof(color));
	

	//return 0;
}


static ssize_t read_xlite_version(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
				uint16_t len, uint16_t offset)
{
	/* APP_VERSION_STRING is generated from the VERSION file, e.g. "1.0.0" */
	const char *ver = APP_VERSION_STRING;
	return bt_gatt_attr_read(conn, attr, buf, len, offset, ver, strlen(ver));
}

static ssize_t read_xlite_vbatt(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			   uint16_t len, uint16_t offset)
{
		vbatt = rBattValue();
		vbatt = vbatt >> 1;
		printf("vbatt sending via ble = %u \n", vbatt);
		//vbatt = 0; 
		return bt_gatt_attr_read(conn, attr, buf, len, offset, &vbatt, sizeof(vbatt));
	

	//return 0;
}

/* STEP 5 - Implement the read callback function of the Button characteristic */



static ssize_t write_xlite_mode(struct bt_conn *conn,
			const struct bt_gatt_attr *attr,
			const void *buf,
			uint16_t len, uint16_t offset, uint8_t flags)
{
	if (len != 1U || offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}
	uint8_t val = *((uint8_t *)buf);
	if (val <= 2) {
		solar_mode = val;
	}
	return len;
}

static ssize_t read_xlite_mode(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &solar_mode, sizeof(solar_mode));
}

/* LED Button Service Declaration */
/* STEP 2 - Create and add the MY LBS service to the Bluetooth LE stack */
BT_GATT_SERVICE_DEFINE(xlite_service,
BT_GATT_PRIMARY_SERVICE(BT_UUID_XLITE),
	BT_GATT_CHARACTERISTIC(BT_UUID_XLITE_COLOR,
			       BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP | BT_GATT_CHRC_READ,
			       BT_GATT_PERM_WRITE | BT_GATT_PERM_READ,
				   read_xlite_color, write_xlite_color, &color),
	BT_GATT_CHARACTERISTIC(BT_UUID_XLITE_BRIGHTNESS,
			       BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP | BT_GATT_CHRC_READ,
			       BT_GATT_PERM_WRITE | BT_GATT_PERM_READ,
			       read_xlite_brightness, write_xlite_brightness, &brightness),
	BT_GATT_CHARACTERISTIC(BT_UUID_XLITE_VBATT,
			       BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, read_xlite_vbatt, NULL,
			       &vbatt),
	BT_GATT_CHARACTERISTIC(BT_UUID_XLITE_MODE,
			       BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP | BT_GATT_CHRC_READ,
			       BT_GATT_PERM_WRITE | BT_GATT_PERM_READ,
			       read_xlite_mode, write_xlite_mode, &solar_mode),
	BT_GATT_CHARACTERISTIC(BT_UUID_XLITE_SOLAR,
			       BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, read_xlite_solar, NULL, &solar_mv),
	BT_GATT_CHARACTERISTIC(BT_UUID_XLITE_VERSION,
			       BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, read_xlite_version, NULL, NULL),
);
/* A function to register application callbacks for the LED and Button characteristics  */
int my_xlite_init(struct my_xlite_cb *callbacks)
{
	if (callbacks) {
		xlite_cb.brightness_cb = callbacks->brightness_cb;
		xlite_cb.color_cb = callbacks->color_cb;
	}

	return 0;
}

