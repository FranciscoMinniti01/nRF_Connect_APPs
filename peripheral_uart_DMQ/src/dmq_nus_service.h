#ifndef BT_NUS_H
#define BT_NUS_H

// INCLUDES ------------------------------------------------------------------------------------------------------------------------

#include <zephyr/types.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include "app_config.h"

//  ------------------------------------------------------------------------------------------------------------------------

#define BT_UUID_NUS_VAL 		  BT_UUID_128_ENCODE(0x6e400001, 0xb5a3, 0xf393, 0xe0a9, 0xe50e24dcca9e)	// UUID del servicio NUS
#define BT_UUID_NUS_RX_VAL 		  BT_UUID_128_ENCODE(0x6e400002, 0xb5a3, 0xf393, 0xe0a9, 0xe50e24dcca9e)	// UUID de la caracteristica RX
#define BT_UUID_NUS_TX_VAL 		  BT_UUID_128_ENCODE(0x6e400003, 0xb5a3, 0xf393, 0xe0a9, 0xe50e24dcca9e)	// UUID de la caracteristica TX
#define BT_UUID_SENSOR_PARAM_VAL  BT_UUID_128_ENCODE(0x6e400004, 0xb5a3, 0xf393, 0xe0a9, 0xe50e24dcca9e)	// UUID de la caracteristica de parametrizacion del sensor

#define BT_UUID_NUS_SERVICE   BT_UUID_DECLARE_128(BT_UUID_NUS_VAL)
#define BT_UUID_NUS_RX        BT_UUID_DECLARE_128(BT_UUID_NUS_RX_VAL)
#define BT_UUID_NUS_TX        BT_UUID_DECLARE_128(BT_UUID_NUS_TX_VAL)
#define BT_UUID_SENSOR_PARAM  BT_UUID_DECLARE_128(BT_UUID_SENSOR_PARAM_VAL)

enum bt_nus_send_status {					// NUS send status.
	BT_NUS_SEND_STATUS_ENABLED,				// Send notification enabled.
	BT_NUS_SEND_STATUS_DISABLED,			// Send notification disabled.
};

typedef struct
{
	/* data */
}dmq_sensor_param_t;

struct bt_nus_cb
{
	void (*received)(struct bt_conn *conn, const uint8_t *const data, uint16_t len); 	// The data has been received as a write request on the NUS RX Characteristic.

	void (*sent)(struct bt_conn *conn);	// The data has been sent as a notification and written on the NUS TX Characteristic. conn can be NULL if sent to all connected peers.

	void (*send_enabled)(enum bt_nus_send_status status);		// Indicate the CCCD descriptor status of the NUS TX characteristic.
};

int bt_nus_init(struct bt_nus_cb *callbacks);

int bt_nus_send(struct bt_conn *conn, const uint8_t *data, uint16_t len);

uint32_t bt_nus_get_mtu(struct bt_conn *conn);

#endif//BT_NUS_H
