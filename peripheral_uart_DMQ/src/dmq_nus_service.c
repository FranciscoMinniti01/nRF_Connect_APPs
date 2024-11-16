// INCLUDES ------------------------------------------------------------------------------------------------------------------------

#include "dmq_nus_service.h"

// VARIABLES ------------------------------------------------------------------------------------------------------------------------

LOG_MODULE_REGISTER(LOG_NUS, LOG_NUS_LEVEL);

static nus_cb_t nus_cb;

// CALLBACKS ------------------------------------------------------------------------------------------------------------------------

static void nus_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	if (nus_cb.send_enabled) {
		LOG_DBG("Notification has been turned %s", ( value == BT_GATT_CCC_NOTIFY ? "ON" : "OFF") );
		nus_cb.send_enabled(value == BT_GATT_CCC_NOTIFY ? true : false);
	}
}

static ssize_t on_receive(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			  			  const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	LOG_DBG("Received data, handle %d, conn %p", attr->handle, (void *)conn);

	if (nus_cb.received) nus_cb.received(conn, buf, len);

	return len;
}

static void on_sent(struct bt_conn *conn, void *user_data)
{
	ARG_UNUSED(user_data);

	LOG_DBG("Data send, conn %p", (void *)conn);

	if (nus_cb.sent) nus_cb.sent(conn);
}

// ATTRIBUTE TABLE ------------------------------------------------------------------------------------------------------------------------

BT_GATT_SERVICE_DEFINE( nus_svc,											// Declaracion del servicio NUS
	BT_GATT_PRIMARY_SERVICE( BT_UUID_NUS_SERVICE),							// Declaracion de la caracteristica que marca el inicio del servicio primario 
	
	BT_GATT_CHARACTERISTIC(	BT_UUID_NUS_TX,									// Declaramos la caracteristica de valor TX y le asignamos su UUID
			       			BT_GATT_CHRC_NOTIFY,								// Idicamos que permite la operacion de notificacion 
							BT_GATT_PERM_READ_AUTHEN,							// Permite la lectura solo con autenticacion
			       			NULL, NULL, 										// Las callback de lectura y escritura no se utilizan
							NULL),												// Datos de usuario del atributo característico.
	
	BT_GATT_CCC( nus_ccc_cfg_changed,										// Esta caracteristica permite configurar la notificacion de la caracteristica TX
				 BT_GATT_PERM_READ_AUTHEN | 									// Permite la lectura con autenticacion
				 BT_GATT_PERM_WRITE_AUTHEN),									// Permite la escritura con autenticacion
	
	BT_GATT_CHARACTERISTIC( BT_UUID_NUS_RX,									// Declaramos la caracteristica de valor RX y le asignamos su UUID
			       			BT_GATT_CHRC_WRITE |								// Operaciones permitidas:	Escritura  
			       			BT_GATT_CHRC_WRITE_WITHOUT_RESP,					// 							Escritura sin confirmacion 
					        BT_GATT_PERM_READ_AUTHEN | 							// Permisos: Lectura con autenticacion
			       			BT_GATT_PERM_WRITE_AUTHEN,							// 			 Escritura con autenticacion
							NULL, on_receive, 									// Configuramos la calback de lectura Y escritura respectivamente
							NULL),												// Datos de usuario del atributo característico.

);

// FUNCTIONS ------------------------------------------------------------------------------------------------------------------------

int bt_nus_init(nus_cb_t *callbacks)
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
	//const struct bt_gatt_attr *attr = &nus_svc.attrs[2];	// Para borrar
	struct bt_gatt_notify_params params = {0};									// Estructura de parametros necesarios para la notificacion

	params.attr = &nus_svc.attrs[2];											// Puntero al atributo de la característica	(Esto puede ser remplazado por &nus_svc.attrs[1] o por )
	params.data = data;															// Puntero tipo void a la informacion a enviar			
	params.len 	= len;															// Largo en bytes de la informacion
	params.func = on_sent;														// Callback de tipo bt_gatt_complete_func_t para confirmar la notificiacion

	if (!conn) return bt_gatt_notify_cb(NULL, &params);							// Si no se expecifico una conexion se notifica a todas las existentes					
	else if (bt_gatt_is_subscribed(conn, params.attr, BT_GATT_CCC_NOTIFY)) {	// Verificaremos manualmente si el cliente tiene habilitada la notificación en esa conexión
		return bt_gatt_notify_cb(conn, &params);								// Si el cliente tiene habilitada la notificacion se le envia una
	}
	else return -EINVAL;
}

uint32_t bt_nus_get_mtu(struct bt_conn *conn)
{
	// According to 3.4.7.1 Handle Value Notification off the ATT protocol. Maximum supported notification is ATT_MTU - 3 */
	return bt_gatt_get_mtu(conn) - 3;											// Devuelve la longitud máxima de datos que se puede enviar con bt_nus_send.
}
