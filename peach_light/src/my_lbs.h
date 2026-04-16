/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_LBS_H_
#define BT_LBS_H_

/**@file
 * @defgroup bt_lbs LED Button Service API
 * @{
 * @brief API for the LED Button Service (LBS).
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/types.h>

/* STEP 1 - Define the 128 bit UUIDs for the GATT service and its characteristics in */

/** @brief XLITE Service UUID. */
#define BT_UUID_XLITE_VAL \
     BT_UUID_128_ENCODE(0xf000BABE, 0x5c80, 0x11ee, 0x8c99, 0x0242ac120002)
/** @brief LED COLOR Characteristic UUID. */
#define BT_UUID_XLITE_COLOR_VAL \
	BT_UUID_128_ENCODE(0xf0001113, 0x5c80, 0x11ee, 0x8c99, 0x0242ac120002)

/** @brief LED BRIGHTNESS Characteristic UUID. */
#define BT_UUID_XLITE_BRIGHTNESS_VAL \
	BT_UUID_128_ENCODE(0xf0001112, 0x5c80, 0x11ee, 0x8c99, 0x0242ac120002)

/** @brief LED BRIGHTNESS Characteristic UUID. */
#define BT_UUID_XLITE_SOLAR_VAL \
	BT_UUID_128_ENCODE(0xf0001110, 0x5c80, 0x11ee, 0x8c99, 0x0242ac120002)

#define BT_UUID_XLITE_VBATT_VAL \
	BT_UUID_128_ENCODE(0xf0001119, 0x5c80, 0x11ee, 0x8c99, 0x0242ac120002)

/** @brief Solar mode characteristic UUID (0=Early/1600mV, 1=Normal/1700mV, 2=Late/1800mV) */
#define BT_UUID_XLITE_MODE_VAL \
	BT_UUID_128_ENCODE(0xf000111A, 0x5c80, 0x11ee, 0x8c99, 0x0242ac120002)

/** @brief Firmware version characteristic UUID — returns null-terminated semver string e.g. "1.0.0" */
#define BT_UUID_XLITE_VERSION_VAL \
	BT_UUID_128_ENCODE(0xf000111B, 0x5c80, 0x11ee, 0x8c99, 0x0242ac120002)


#define BT_UUID_XLITE		       BT_UUID_DECLARE_128(BT_UUID_XLITE_VAL)
#define BT_UUID_XLITE_COLOR		   BT_UUID_DECLARE_128(BT_UUID_XLITE_COLOR_VAL)
#define BT_UUID_XLITE_BRIGHTNESS   BT_UUID_DECLARE_128(BT_UUID_XLITE_BRIGHTNESS_VAL)
#define BT_UUID_XLITE_SOLAR		   BT_UUID_DECLARE_128(BT_UUID_XLITE_SOLAR_VAL)
#define BT_UUID_XLITE_VBATT		   BT_UUID_DECLARE_128(BT_UUID_XLITE_VBATT_VAL)
#define BT_UUID_XLITE_MODE		   BT_UUID_DECLARE_128(BT_UUID_XLITE_MODE_VAL)
#define BT_UUID_XLITE_VERSION	   BT_UUID_DECLARE_128(BT_UUID_XLITE_VERSION_VAL)


/** @brief Callback type for when an LED state change is received. */
typedef void (*brightness_cb_t)(uint8_t brightness);

/** @brief Callback type for when the color is set. */
typedef void (*color_cb_t)(uint8_t color);

/** @brief Callback struct used by the LBS Service. */
struct my_xlite_cb {
	/** LED state change callback. */
	brightness_cb_t brightness_cb;
	/** Button read callback. */
	color_cb_t color_cb;
};

/** @brief Initialize the LBS Service.
 *
 * This function registers application callback functions with the My LBS
 * Service
 *
 * @param[in] callbacks Struct containing pointers to callback functions
 *			used by the service. This pointer can be NULL
 *			if no callback functions are defined.
 *
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int my_xlite_init(struct my_xlite_cb *callbacks);

/** @brief Get the solar threshold in mV based on the current mode.
 *  0 = Early (1600mV), 1 = Normal (1700mV), 2 = Late (1800mV)
 */
int32_t getSolarThreshold(void);

/** @brief Store the latest solar panel voltage (mV) for BLE reads. */
void setSolarValue(int32_t mv);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_LBS_H_ */
