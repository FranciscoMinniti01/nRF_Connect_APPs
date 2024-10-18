// INCLUDES ------------------------------------------------------------------------------------------------------------------------

#include "ble_machine.h"

LOG_MODULE_REGISTER(LOG_BLE,LOG_LEVEL);			                // Registro del modulo logger y configuracion del nivel

// GLOBAL VARIABLES ------------------------------------------------------------------------------------------------------------------------

static uint8_t ble_machine_state    = BLE_CB_REGISTERS;	        // Estado de la maquina de estado BLE
static bool app_notifi_error        = false;                    // Indicador de error en la maquina de estado
static bool app_notifi_adv          = false;                    // Indicador de estado de advertising
static bool app_notifi_is_conn      = false;                    // Indicador del estado de la coneccion

static bool is_update_param         = false;
static bool is_update_PHY           = false;
struct bt_conn *current_conn        = NULL;

// ADVERTISING ------------------------------------------------------------------------------------------------------------------------

static const struct bt_data ad[] = {                                        // Estructura del Advertising package con la informacion que contiene
	BT_DATA_BYTES( BT_DATA_FLAGS, ADV_PAC_FLAGS),                           // Configuracion de las banderas del paquete
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, sizeof(DEVICE_NAME) - 1),   // Incluye en el paquete el nombre del dispositivo que vera el usuario
};

static const struct bt_data sd[] = {                                        // Estructura del scan packet con la informacion que contiene
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_NUS_VAL),                    // Incluye el UUID del servicio NUS
};

void advertising_start()
{
    int err;
	struct bt_le_adv_param *adv_param = BT_LE_ADV_PARAM( ADV_OPTIONS, ADV_MIN_INTERVAL,	ADV_MAX_INTERVAL, NULL);        // Estructura con los parametros de publicidad

	err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));				                            // Inicia la publicidad
	if (err) {
		if (err == -EALREADY) LOG_INF("Advertising continued\n");         // Esto nose si es necesario, lo dejo para ver cuando pasa, Tengo que probar con BT_LE_ADV_OPT_ONE_TIME en eñ adv_param. FRAN
		else LOG_ERR("Advertising failed to start. Error: %d\n", err);
		return;
	}

	app_notifi_adv = true;
	LOG_INF("Advertising successfully started\n");
}

// CONNECTIONS ------------------------------------------------------------------------------------------------------------------------

void connected(struct bt_conn *conn, uint8_t err)
{
    if (err) {
        LOG_ERR("Connection failed. Error: %d\n", err);
        return;
    }

    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_INF("Connected %s", addr);

    struct bt_conn_info info;
	err = bt_conn_get_info(conn, &info);                                        // Optengo e imprimo los parametros iniciales de coneccion
	if (err) LOG_WRN("Get param of connection failed. Error: %d\n", err);
    else LOG_INF("Connection param: interval %.2f ms, latency %d intervals, timeout %d ms", info.le.interval*1.25, info.le.latency, info.le.timeout*10);
    
    current_conn = bt_conn_ref(conn);

    app_notifi_is_conn = true;
    ble_machine_state = BLE_UPDATE_CONN_PARAM;
}

void disconnected(struct bt_conn *conn, uint8_t reason)
{
    LOG_INF("Disconnected. Reason: %d\n", reason);
    app_notifi_is_conn = false;

    if (current_conn) {
		bt_conn_unref(current_conn);
		current_conn = NULL;
	}

    ble_machine_state = BLE_ADVERTISING;
}

void param_updated(struct bt_conn *conn, uint16_t interval, uint16_t latency, uint16_t timeout)
{
    LOG_INF("Conn param update: interval %.2f ms, latency %d intervals, timeout %d ms", interval*1.25, latency, timeout*10);
    is_update_param = true;
}

void phy_updated(struct bt_conn *conn, struct bt_conn_le_phy_info *param)
{
    if (param->tx_phy == BT_CONN_LE_TX_POWER_PHY_1M) {
        LOG_INF("PHY updated. New PHY: 1M");
    }
    else if (param->tx_phy == BT_CONN_LE_TX_POWER_PHY_2M) {
        LOG_INF("PHY updated. New PHY: 2M");
    }
    else if (param->tx_phy == BT_CONN_LE_TX_POWER_PHY_CODED_S8) {
        LOG_INF("PHY updated. New PHY: Long Range");
    }
    is_update_PHY = true;
}

void security_changed(struct bt_conn *conn, bt_security_t level,enum bt_security_err err)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    if (!err) LOG_INF("Security changed: %s level %u\n", addr, level);
    else LOG_INF("Security failed: %s level %u err %d\n", addr, level, err);
}

struct bt_conn_cb connection_cb = {
    .connected              = connected,
    .disconnected           = disconnected,
    .le_param_updated       = param_updated,
    .le_phy_updated         = phy_updated,
    .security_changed       = security_changed
};

// SECURITY ------------------------------------------------------------------------------------------------------------------------

static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Passkey for %s: %06u\n", addr, passkey);
}

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Pairing cancelled: %s\n", addr);
}

static struct bt_conn_auth_cb conn_auth_callbacks = {
	.passkey_display = auth_passkey_display,
	.cancel = auth_cancel,
};

// FUNCTIONS ------------------------------------------------------------------------------------------------------------------------

bool get_BLE_state_conn()
{
    return app_notifi_is_conn;
}

bool get_BLE_state_adv()
{
    return app_notifi_adv;
}

bool get_BEL_state_error()
{
    return app_notifi_error;
}

// MACHINE ------------------------------------------------------------------------------------------------------------------------

void ble_machine()
{
    uint8_t err;
    switch (ble_machine_state)
    {
        case BLE_CB_REGISTERS:
        {	
            err = bt_conn_auth_cb_register(&conn_auth_callbacks);
            if (err) {
                LOG_INF("Failed to register authorization callbacks.\n");
                return;
            }

            bt_conn_cb_register(&connection_cb);
            ble_machine_state = BLE_INIT;
        }

        case BLE_INIT:
        {
            err = bt_enable(NULL);                                          // Habilita e inicializa la pila BLE
	        if (err) {
		        LOG_ERR("BLE Bluetooth init failed. Error: %d\n", err);
				app_notifi_error = true;
		        return;
	        }

            // Rever el funcionamiento de ese bloque FRAN
            bt_addr_le_t perypheral_addr;                                   // Variable para almacenar mi direccion BLE
            err = bt_addr_le_create_static(&perypheral_addr);               // Creacion de una BLE random static address
            if(err){
                LOG_ERR("BLE addr create failed, Error: %d\n", err);
                app_notifi_error = true;
            } else {
                if(bt_id_create(&perypheral_addr,NULL) < 0) {               // Agregamos la direccion
                    LOG_ERR("BLE addr add failed, Error: %d\n", err);
                    app_notifi_error = true;
                }
            }
            char addr[BT_ADDR_LE_STR_LEN];
            bt_addr_le_to_str(&perypheral_addr, addr, sizeof(addr));        // Combierto la direccion a string para imprimirla
            LOG_INF("Perypheral addr: %s\n",addr);

			/*if (IS_ENABLED(CONFIG_SETTINGS)) {
		        settings_load();
	        }*/

            bt_passkey_set(PASSKEY);

			app_notifi_error = false;
            LOG_INF("BLE inicializado\n");
            ble_machine_state = BLE_ADVERTISING;
            break;
        }

        case BLE_ADVERTISING:
        {
	        advertising_start();
            ble_machine_state = BLE_WAITING_CONECTION;
            break;
        }

        case BLE_WAITING_CONECTION:
        {
            if(is_update_param && is_update_PHY){
                ble_machine_state = BLE_READY;
            }
            break;
        }

        case BLE_UPDATE_CONN_PARAM:
        {
            int err;
            struct bt_le_conn_param *conn_param = BT_LE_CONN_PARAM(CONN_MIN_INTERVAL,CONN_MAN_INTERVAL,CONN_LATENCY,CONN_TIMEOUT);
            err = bt_conn_le_param_update(current_conn,conn_param);
            if(err){
                LOG_ERR("BLE update param failed, Error: %d\n", err);
                app_notifi_error = true;
            }

            const struct bt_conn_le_phy_param preferred_phy = {
                .options = BT_CONN_LE_PHY_OPT_NONE,
                .pref_rx_phy = BT_GAP_LE_PHY_2M,
                .pref_tx_phy = BT_GAP_LE_PHY_2M,
            };
            err = bt_conn_le_phy_update(current_conn, &preferred_phy);
            if (err) {
                LOG_ERR("bt_conn_le_phy_update() returned %d", err);
                app_notifi_error = true;
            }

            //Aca podria agrandar el tamaño de la UART L3E3 punto 9 FRAN

            ble_machine_state = BLE_WAITING_CONECTION;
            break;
        }

        case BLE_READY:
        {
            LOG_INF("BLE ready");
            ble_machine_state = BLE_WAITING;
        }

        case BLE_WAITING:
        {
            break;
        }
		
    }
}