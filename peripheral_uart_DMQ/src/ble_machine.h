#ifndef BLE_MACHINE_H
#define BLE_MACHINE_H

// INCLUDES ------------------------------------------------------------------------------------------------------------------------

#include <zephyr/bluetooth/bluetooth.h>         // Incluye definiciones, macros y las funciones principales de la pila BLE como bt_enable, bt_le_adv_start. 
#include <zephyr/bluetooth/gap.h>               // Incluye las definiciones de las opciones disponiple para la etapa del advertisign
#include <zephyr/bluetooth/uuid.h>              // Permite el manejo de las UUID de servicios
#include <zephyr/bluetooth/addr.h>              // Permite administrar las direcciones BLE
#include <zephyr/bluetooth/conn.h>              // Permite el manejo de la conexión BLE

#include "dmq_nus_service.h"                    // Incluye las funciones y definiciones particulares del servicio NUS utilizando el codigo modificado
#include "app_config.h"

// DEFINITIONS ------------------------------------------------------------------------------------------------------------------------

enum ble_machine
{
    BLE_CB_REGISTERS,
    BLE_INIT,
    BLE_ADVERTISING,
    BLE_WAITING_CONECTION,
    BLE_CONECCTED,
    BLE_WAITING_TO_SEND,
    KEY_REPORT_SENT,
    BLE_WAITING_SEND,
    BLE_CLEAR_KEY,
    BLE_DEACTIVATE,

    BLE_WAITING,
    BLE_READY,
    BLE_UPDATE_CONN_PARAM
};

// FUNCTIONS ------------------------------------------------------------------------------------------------------------------------

void advertising_start();

void connected(struct bt_conn *conn, uint8_t err);
void disconnected(struct bt_conn *conn, uint8_t reason);
void param_updated(struct bt_conn *conn, uint16_t interval, uint16_t latency, uint16_t timeout);
void phy_updated(struct bt_conn *conn, struct bt_conn_le_phy_info *param);
void security_changed(struct bt_conn *conn, bt_security_t level,enum bt_security_err err);

bool get_BLE_state_conn();
bool get_BLE_state_adv();
bool get_BEL_state_error();

void ble_machine();

#endif//BLE_MACHINE_H