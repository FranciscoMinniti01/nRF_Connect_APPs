#ifndef CLIENT_BLE_MACHINE_H
#define CLIENT_BLE_MACHINE_H

// INCLUDES --------------------------------------------------------------------------------------------------------------------------------------------------------------------------

#include <zephyr/bluetooth/bluetooth.h>

#include <zephyr/bluetooth/conn.h>                  // Permite el manejo de la conexión BLE
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include <zephyr/settings/settings.h>

#include <bluetooth/services/nus.h>
#include <bluetooth/services/nus_client.h>
#include <bluetooth/gatt_dm.h>
#include <bluetooth/scan.h>

#include "APP_Config.h"

// DEFINITIONS --------------------------------------------------------------------------------------------------------------------------------------------------------------------------

#define LOG_BLE_LEVEL 3

enum Client_BLE_States
{
    CLIENT_BLE_INIT,
    CLIENT_BLE_WAIT_CONNECTION,
    CLIENT_BLE_CONNECTED,
};

#define GET_BT_ADDR_STR(origin, addr)                                     \
        char addr[BT_ADDR_LE_STR_LEN];                                  \
        bt_addr_le_to_str(origin, addr, sizeof(addr))

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------

static void gatt_discover(struct bt_conn *conn);

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------

#endif//CLIENT_BLE_MACHINE_H
