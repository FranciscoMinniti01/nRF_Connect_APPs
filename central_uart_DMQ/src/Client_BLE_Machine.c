// INCLUDES --------------------------------------------------------------------------------------------------------------------------------------------------------------------------

#include "Client_BLE_Machine.h"

// VARIABLES --------------------------------------------------------------------------------------------------------------------------------------------------------------------------

LOG_MODULE_REGISTER(LOG_BLE,LOG_BLE_LEVEL);			            // Registro del modulo logger y configuracion del nivel

static uint8_t Client_BLE_State;	                            // Estado de la maquina de estado BLE

static struct bt_conn *default_conn;
static struct bt_nus_client nus_client;

// SCANING --------------------------------------------------------------------------------------------------------------------------------------------------------------------------

static void scan_filter_match( struct bt_scan_device_info *device_info,
			                   struct bt_scan_filter_match *filter_match,
			                   bool connectable )
{
    GET_BT_ADDR_STR(device_info->recv_info->addr, addr);
	LOG_INF("Scan filters matched. Address: %s connectable: %d", addr, connectable);
}

static void scan_connecting_error(struct bt_scan_device_info *device_info)
{
	GET_BT_ADDR_STR(device_info->recv_info->addr, addr);
	LOG_WRN("Scan connecting failed. Address: %s", addr);
}

static void scan_connecting(struct bt_scan_device_info *device_info, struct bt_conn *conn)
{
	default_conn = bt_conn_ref(conn);
	GET_BT_ADDR_STR(bt_conn_get_dst(conn), addr);
	LOG_INF("Scan successful, connection is started. Address: %s", addr);
}

struct cb_data scan_cb_data = {
	.filter_match 		= scan_filter_match,
	.connecting_error 	= scan_connecting_error,
	.connecting 		= scan_connecting
};
static struct bt_scan_cb scan_cb = {
	.cb_addr = &scan_cb_data,	
};

static int scan_init(uint16_t unique_id)
{
	int err;

	struct bt_le_scan_param scan_param_init = {
		.options 	= BT_LE_SCAN_OPT_FILTER_DUPLICATE,								// Evita el procesamiento repetido de paquetes de publicidad provenientes de un mismo periferico. 			
		.interval 	= 0x0060,
		.window 	= 0x0050,
		.timeout 	= 0,
	};

	struct bt_scan_init_param scan_init = {
		.scan_param = &scan_param_init,
		.connect_if_match = 1,														// Configuracion que perimite iniciar una coneccion con los dispositivos que cumplen con el filtro
	};

	bt_scan_init(&scan_init);														// Inicializamos el modulo scan pero no el scanning
	
	bt_scan_cb_register(&scan_cb);													// Registramos las callbacks de scan

	err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID, BT_UUID_NUS_SERVICE);		// Creamos un filtro para el UUID del servicio NUS
	if (err) {
		LOG_ERR("Scanning filters cannot be set (err %d)", err);
		return err;
	}

	err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_MANUFACTURER_DATA, unique_id);		// Creamos un filtro para el 
	if (err) {
		LOG_ERR("Scanning filters cannot be set (err %d)", err);
		return err;
	}

	err = bt_scan_filter_enable(BT_SCAN_ALL_FILTER, true);							// Habilitamos los filtros, BT_SCAN_ALL_FILTER: indicamos que habilitamos todos los filtros. true: Indicamos que deben coincidir todos a la vez 
	if (err) {
		LOG_ERR("Filters cannot be turned on (err %d)", err);
		return err;
	}

	LOG_INF("Scan module initialized");
	return err;
}


// CONNECTIONS --------------------------------------------------------------------------------------------------------------------------------------------------------------------------

static void exchange_func(struct bt_conn *conn, uint8_t err, struct bt_gatt_exchange_params *params)
{
	if (!err) LOG_INF("MTU exchange done");
	else LOG_WRN("MTU exchange failed (err %" PRIu8 ")", err);
}

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	int err;
    GET_BT_ADDR_STR(bt_conn_get_dst(conn), addr);

	if(conn_err)
	{
		LOG_INF("Failed to connect to %s (%d)", addr, conn_err);
		if (default_conn == conn)
		{
			bt_conn_unref(default_conn);
			default_conn = NULL;
			err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
			if (err) {
				LOG_ERR("Scan start failed (err %d)",err);
			}
		}
		return;
	}

	static struct bt_gatt_exchange_params exchange_params;      //  Estructura para la negociacion del tamaño del MTU
	exchange_params.func = exchange_func;                       //  Callback para notificar la finalizacion de la notificacion 
	err = bt_gatt_exchange_mtu(conn, &exchange_params);         //  Llama a la negociacion del tamañano del MTU
	if (err) LOG_WRN("MTU exchange failed (err %d)", err);      //  La MTU se configura en el prj.conf creo // FRAN

	err = bt_conn_set_security(conn, BT_SECURITY_L3);
	if (err) {
		LOG_WRN("Failed to set security: %d", err);
		gatt_discover(conn);
	}

	err = bt_scan_stop();
	if ((!err) && (err != -EALREADY)) {
		LOG_ERR("Stop LE scan failed (err %d)", err);
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	int err;

	GET_BT_ADDR_STR(bt_conn_get_dst(conn), addr);

	LOG_INF("Disconnected: %s (reason %u)", addr, reason);

	if (default_conn != conn) {
		return;
	}

	bt_conn_unref(default_conn);
	default_conn = NULL;

	err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
	if (err) {
		LOG_ERR("Scanning failed to start (err %d)",
			err);
	}
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		LOG_INF("Security changed: %s level %u", addr, level);
	} else {
		LOG_WRN("Security failed: %s level %u err %d", addr,
			level, err);
	}

	gatt_discover(conn);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed
};


// GATT DISCOVERY --------------------------------------------------------------------------------------------------------------------------------------------------------------------------

static void discovery_complete(struct bt_gatt_dm *dm,
			       void *context)
{
	struct bt_nus_client *nus = context;
	LOG_INF("Service discovery completed");

	bt_gatt_dm_data_print(dm);

	bt_nus_handles_assign(dm, nus);
	bt_nus_subscribe_receive(nus);

	bt_gatt_dm_data_release(dm);
}

static void discovery_service_not_found(struct bt_conn *conn,
					void *context)
{
	LOG_INF("Service not found");
}

static void discovery_error(struct bt_conn *conn,
			    int err,
			    void *context)
{
	LOG_WRN("Error while discovering GATT database: (%d)", err);
}

struct bt_gatt_dm_cb discovery_cb = {
	.completed         = discovery_complete,
	.service_not_found = discovery_service_not_found,
	.error_found       = discovery_error,
};

static void gatt_discover(struct bt_conn *conn)
{
	int err;

	if (conn != default_conn) {
		return;
	}

	err = bt_gatt_dm_start(conn,
			       BT_UUID_NUS_SERVICE,
			       &discovery_cb,
			       &nus_client);
	if (err) {
		LOG_ERR("could not start the discovery procedure, error "
			"code: %d", err);
	}
}


// AUTHENTUCATION AND PAIRING --------------------------------------------------------------------------------------------------------------------------------------------------------------------------

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_INF("Pairing cancelled: %s", addr);
}

static struct bt_conn_auth_cb conn_auth_callbacks = {
	.cancel = auth_cancel,
};


static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	char addr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_INF("Pairing completed: %s, bonded: %d", addr, bonded);
}

static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	char addr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_WRN("Pairing failed conn: %s, reason %d", addr, reason);
}

static struct bt_conn_auth_info_cb conn_auth_info_callbacks = {
	.pairing_complete = pairing_complete,
	.pairing_failed = pairing_failed
};

// NUS --------------------------------------------------------------------------------------------------------------------------------------------------------------------------

static void ble_data_sent(struct bt_nus_client *nus, uint8_t err,
					const uint8_t *const data, uint16_t len)
{
	ARG_UNUSED(nus);
	if (err) LOG_WRN("ATT error code: 0x%02X", err);
}

static uint8_t ble_data_received(struct bt_nus_client *nus,
						const uint8_t *data, uint16_t len)
{
	ARG_UNUSED(nus);
	return BT_GATT_ITER_CONTINUE;
}

static int nus_client_init(void)
{
	int err;
	struct bt_nus_client_init_param init = {
		.cb = {
			.received = ble_data_received,
			.sent = ble_data_sent,
		}
	};

	err = bt_nus_client_init(&nus_client, &init);
	if (err) {
		LOG_ERR("NUS Client initialization failed (err %d)", err);
		return err;
	}

	LOG_INF("NUS Client module initialized");
	return err;
}

// MACHINE --------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Central_BLE_Machine()
{
    uint8_t err;
    switch (Client_BLE_State)
    {
        case CLIENT_BLE_INIT:
        {
            err = bt_conn_auth_cb_register(&conn_auth_callbacks);
            if (err) {
                LOG_ERR("Failed to register authorization callbacks.");
            }

            err = bt_conn_auth_info_cb_register(&conn_auth_info_callbacks);
            if (err) {
                printk("Failed to register authorization info callbacks.\n");
            }

            err = bt_enable(NULL);
            if (err) {
                LOG_ERR("Bluetooth init failed (err %d)", err);
            }

            if (IS_ENABLED(CONFIG_SETTINGS)) {
                settings_load();
            }

            err = scan_init();
            if (err) {
                LOG_ERR("scan_init failed (err %d)", err);
            }

            err = nus_client_init();
            if (err) {
                LOG_ERR("nus_client_init failed (err %d)", err);
            }

            err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
            if (err) {
                LOG_ERR("Scanning failed to start (err %d)", err);
            }

            Client_BLE_State = CLIENT_BLE_WAIT_CONNECTION;
            break;
        }

        case CLIENT_BLE_WAIT_CONNECTION:
        {
            break;
        }

        case CLIENT_BLE_CONNECTED:
        {
            LOG_INF("Client BLE Connection");
            break;
        }

    }

}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------