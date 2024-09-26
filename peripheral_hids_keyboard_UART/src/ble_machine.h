#ifndef BLE_MACHINE_H
#define BLE_MACHINE_H

// INCLUDES ------------------------------------------------------------------------------------------------------------------------

#include <zephyr/bluetooth/conn.h>          // Para las estructuras de callbacks de pairing y authenticacion
#include <zephyr/bluetooth/bluetooth.h>     // APIs de Generic Access Profile (GAP) , bt_enable() - advertising_start ...
#include <bluetooth/services/hids.h>        // Para la funcion hid_init en principio
#include <zephyr/settings/settings.h>       // Lo agrege por la funcion settings_load() del estado BLE_ADVERTISING

#include "APP_conf.h"

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
    BLE_DEACTIVATE
};

#define DEVICE_NAME     CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

// CALLBAKS ------------------------------------------------------------------------------------------------------------------------

void auth_passkey_display(struct bt_conn *conn, unsigned int passkey);
void auth_passkey_confirm(struct bt_conn *conn, unsigned int passkey);
void auth_cancel(struct bt_conn *conn);

void pairing_complete(struct bt_conn *conn, bool bonded);
void pairing_failed(struct bt_conn *conn, enum bt_security_err reason);
void pairing_process(struct k_work *work);

void connected(struct bt_conn *conn, uint8_t err);
void disconnected(struct bt_conn *conn, uint8_t reason);
void security_changed(struct bt_conn *conn, bt_security_t level,enum bt_security_err err);

//  ------------------------------------------------------------------------------------------------------------------------

void ble_machine();

//  ------------------------------------------------------------------------------------------------------------------------

bool get_BLE_state_conection();

bool get_state_adv();

bool get_BLE_notifi_error(void);

typedef void (*string_complete_cb_t) ();
void Report_complete_callback(struct bt_conn *conn, void *user_data);

bool string_send(char* cadena, int8_t len,string_complete_cb_t);

int hid_kbd_state_key_set(uint8_t key);

int hid_kbd_state_key_clear(uint8_t key);

int key_report_send(void);

//------------------------------------------------------------------------------------------------------------------------

#endif // BLE_MACHINE_H