// INCLUDES ------------------------------------------------------------------------------------------------------------------------

#include "dmq_nus_service.h"

LOG_MODULE_REGISTER(LOG_NUS, LOG_LEVEL);

static struct bt_nus_cb nus_cb;

static void nus_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	if (nus_cb.send_enabled) {
		LOG_DBG("Notification has been turned %s", ( value == BT_GATT_CCC_NOTIFY ? "ON" : "OFF") );
		nus_cb.send_enabled(value == BT_GATT_CCC_NOTIFY ? BT_NUS_SEND_STATUS_ENABLED : BT_NUS_SEND_STATUS_DISABLED);
	}
}

static ssize_t on_receive(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			  			  const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	LOG_DBG("Received data, handle %d, conn %p",
		attr->handle, (void *)conn);

	if (nus_cb.received) {
		nus_cb.received(conn, buf, len);
	}
	return len;
}

static void on_sent(struct bt_conn *conn, void *user_data)
{
	ARG_UNUSED(user_data);

	LOG_DBG("Data send, conn %p", (void *)conn);

	if (nus_cb.sent) {
		nus_cb.sent(conn);
	}
}

BT_GATT_SERVICE_DEFINE( nus_svc,								// Declaracion del servicio NUS
	BT_GATT_PRIMARY_SERVICE( BT_UUID_NUS_SERVICE),				// Declaracion de la caracteristica que marca el inicio del servicio primario 
	
	BT_GATT_CHARACTERISTIC(	BT_UUID_NUS_TX,						// Declaramos la caracteristica de valor TX y le asignamos su UUID
			       			BT_GATT_CHRC_NOTIFY,					// Idicamos que permite la operacion de notificacion 
							BT_GATT_PERM_READ_AUTHEN,				// Permite la lectura sin incriptacion ni autenticacion
			       			NULL, NULL, 							// Las callback de lectura y escritura no se utilizan
							NULL),									// Datos de usuario del atributo característico.
	
	BT_GATT_CCC( nus_ccc_cfg_changed,							// Esta caracteristica permite configurar la notificacion de la caracteristica TX
				 BT_GATT_PERM_READ_AUTHEN | 						// Permite la lectura con autenticacion
				 BT_GATT_PERM_WRITE_AUTHEN),						// Permite la escritura con autenticacion
	
	BT_GATT_CHARACTERISTIC( BT_UUID_NUS_RX,						// Declaramos la caracteristica de valor RX y le asignamos su UUID
			       			BT_GATT_CHRC_WRITE |					// Idicamos que permite la operacion de escritura  
			       			BT_GATT_CHRC_WRITE_WITHOUT_RESP,		// Idicamos que permite la operacion de escritura sin notificacion 
					        BT_GATT_PERM_READ_AUTHEN | 				// Permite la lectura con autenticacion
			       			BT_GATT_PERM_WRITE_AUTHEN,				// Permite la escritura con autenticacion
							NULL, on_receive, 						// Configuramos la calback de lectura en NULL y la de escritura
							NULL),									// Datos de usuario del atributo característico.
);

int bt_nus_init(struct bt_nus_cb *callbacks)
{
	if (callbacks) {
		nus_cb.received = callbacks->received;
		nus_cb.sent = callbacks->sent;
		nus_cb.send_enabled = callbacks->send_enabled;
	}
	return 0;
}

int bt_nus_send(struct bt_conn *conn, const uint8_t *data, uint16_t len)
{
	struct bt_gatt_notify_params params = {0};
	const struct bt_gatt_attr *attr = &nus_svc.attrs[2];

	params.attr = attr;
	params.data = data;
	params.len = len;
	params.func = on_sent;

	if (!conn) {
		LOG_DBG("Notification send to all connected peers");
		return bt_gatt_notify_cb(NULL, &params);
	} else if (bt_gatt_is_subscribed(conn, attr, BT_GATT_CCC_NOTIFY)) {
		return bt_gatt_notify_cb(conn, &params);
	} else {
		return -EINVAL;
	}
}

uint32_t bt_nus_get_mtu(struct bt_conn *conn)
{
	/* According to 3.4.7.1 Handle Value Notification off the ATT protocol.
	 * Maximum supported notification is ATT_MTU - 3 */
	return bt_gatt_get_mtu(conn) - 3;	// Get maximum data length that can be used for @ref bt_nus_send.
}
