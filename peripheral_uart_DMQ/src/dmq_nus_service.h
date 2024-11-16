#ifndef BT_NUS_H
#define BT_NUS_H

// INCLUDES ------------------------------------------------------------------------------------------------------------------------

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include "app_config.h"

// UUID ------------------------------------------------------------------------------------------------------------------------

#define BT_UUID_NUS_VAL 		BT_UUID_128_ENCODE(0x6e400001, 0xb5a3, 0xf393, 0xe0a9, 0xe50e24dcca9e)	// UUID del servicio NUS
#define BT_UUID_NUS_RX_VAL 		BT_UUID_128_ENCODE(0x6e400002, 0xb5a3, 0xf393, 0xe0a9, 0xe50e24dcca9e)	// UUID de la caracteristica RX
#define BT_UUID_NUS_TX_VAL 		BT_UUID_128_ENCODE(0x6e400003, 0xb5a3, 0xf393, 0xe0a9, 0xe50e24dcca9e)	// UUID de la caracteristica TX

#define BT_UUID_NUS_SERVICE   	BT_UUID_DECLARE_128(BT_UUID_NUS_VAL)
#define BT_UUID_NUS_RX        	BT_UUID_DECLARE_128(BT_UUID_NUS_RX_VAL)
#define BT_UUID_NUS_TX        	BT_UUID_DECLARE_128(BT_UUID_NUS_TX_VAL)

// VARIABLES ------------------------------------------------------------------------------------------------------------------------

typedef struct {
	void (*received)(struct bt_conn *conn, const uint8_t *const data, uint16_t len); 	// Callback para notificar la recepcion de informacion.

	void (*sent)(struct bt_conn *conn);													// Callback para confirmar el envio de la informacion.

	void (*send_enabled)(bool status);								// Callback para notificar el estado de habilitacion del envio de informacion
}nus_cb_t;

// FUNCIONES ------------------------------------------------------------------------------------------------------------------------

int bt_nus_init(nus_cb_t *callbacks);

int bt_nus_send(struct bt_conn *conn, const uint8_t *data, uint16_t len);

uint32_t bt_nus_get_mtu(struct bt_conn *conn);

// ------------------------------------------------------------------------------------------------------------------------

#endif//BT_NUS_H
