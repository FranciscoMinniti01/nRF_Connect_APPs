// INCLUDES ------------------------------------------------------------------------------------------------------------------------

#include "ble_machine.h"

//LOG_DBG
//LOG_INF
//LOG_WRN
//LOG_ERR

// VARIABLES ------------------------------------------------------------------------------------------------------------------------

static uint8_t ble_machine_state = BLE_CB_REGISTERS;	//Estado de la maquina de estado BLE

int8_t global_conter = 0;
bool reconeccted_flag = true;
bool state_conection = false;
bool app_notifi_error_b = false;	
static volatile bool is_adv = false; 							//INDICA SI ESTA EN ADVERTISING

string_complete_cb_t string_complete_cb;
int8_t* sending_string_pointer;
int8_t 	sending_string_len;



/*---------------------------------------- VARIABLES DEL PROCEDIMIENTO --------------------------------------------------------------*/

//ESTO LO PUSE POR LA FUNCION num_comp_reply Y TIENE QUE VER CON LA CLAVE
static struct k_work pairing_work;
struct pairing_data_mitm {
	struct bt_conn *conn;
	unsigned int passkey;
};

//ESTO LO PUSE POR LA FUNCION num_comp_reply Y TIENE QUE VER CON LOS PROCESOS DEL SISTEMA OPERATIVO
K_MSGQ_DEFINE(mitm_queue,
	      sizeof(struct pairing_data_mitm),
	      CONFIG_BT_HIDS_MAX_CLIENT_COUNT,
	      4);

static const struct bt_data ad[] = {											// Estructura del Advertising packet
	BT_DATA_BYTES( 	BT_DATA_GAP_APPEARANCE,										// Da informacion de como deberia verse fisicamente el dispositivo (PARA BORRAR)
		      		(CONFIG_BT_DEVICE_APPEARANCE >> 0) & 0xff,
		      		(CONFIG_BT_DEVICE_APPEARANCE >> 8) & 0xff	),
	BT_DATA_BYTES(	BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)	), 	// Banderas que indican que el dispositivo no es compatible con Bluetooth clásico (BR/EDR) y que el periodo de publicidad es prolongado.
	BT_DATA_BYTES(	BT_DATA_UUID16_ALL, 										// Indica todos los UUIDs de 16 bits que el dispositivo está publicitando
					BT_UUID_16_ENCODE(BT_UUID_HIDS_VAL),							// Representa el UUID del servicio Human Interface Device Service (HIDS)
					BT_UUID_16_ENCODE(BT_UUID_BAS_VAL)	),							// Representa el UUID del servicio Battery Service (BAS)
};

// Informacion y estructura del  scan response
static const struct bt_data sd[] = {											// Estructura de la respuesta de escaneo
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),				// Incluye el nombre del dispositivo que el usuario humano ve cuando busca dispositivos cercanos
};

/*---------------------------------------- ESTUCTURAS DE CALLBACKS --------------------------------------------------------------------------------*/

static struct bt_conn_auth_cb conn_auth_callbacks = {
	.passkey_display = auth_passkey_display,
	.passkey_confirm = auth_passkey_confirm,
	.cancel = auth_cancel,
};

static struct bt_conn_auth_info_cb conn_auth_info_callbacks = {
	.pairing_complete = pairing_complete,
	.pairing_failed = pairing_failed
};

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed,
};

/*---------------------------------------- HID VARIABLES AND DEFINES --------------------------------------------------------------------------------*/

#define BASE_USB_HID_SPEC_VERSION   0x0101

#define OUTPUT_REPORT_MAX_LEN            1
#define OUTPUT_REPORT_BIT_MASK_CAPS_LOCK 0x02
#define INPUT_REP_KEYS_REF_ID            0
#define OUTPUT_REP_KEYS_REF_ID           0
#define MODIFIER_KEY_POS                 0
#define SHIFT_KEY_CODE                   0x02
#define SCAN_CODE_POS                    2
#define KEYS_MAX_LEN                    (INPUT_REPORT_KEYS_MAX_LEN - \
					SCAN_CODE_POS)

#define INPUT_REPORT_KEYS_MAX_LEN (1 + 1 + KEY_PRESS_MAX)

enum {
	OUTPUT_REP_KEYS_IDX = 0
};

enum {
	INPUT_REP_KEYS_IDX = 0
};

// ESTAS DEFINICIONES LAS PUSE POR LAS FUNCIONES DE QUE ENVIAN EL REPORTE
#define KEY_CTRL_CODE_MIN 224 /* Control key codes - required 8 of them */
#define KEY_CTRL_CODE_MAX 231 /* Control key codes - required 8 of them */
#define KEY_CODE_MIN      0   /* Normal key codes */
#define KEY_CODE_MAX      101 /* Normal key codes */
#define KEY_PRESS_MAX     6   /* Maximum number of non-control keyspressed simultaneously*/ //FRAN: No se puede aumentar este numero

BT_HIDS_DEF(hids_obj, OUTPUT_REPORT_MAX_LEN, INPUT_REPORT_KEYS_MAX_LEN); // HIDS instance

//ESTO SE USA EN LAS FUNCIONES DE CONNECTED Y EN LAS DE HID
static struct conn_mode {
	struct bt_conn *conn;
	bool in_boot_mode;
} conn_mode[CONFIG_BT_HIDS_MAX_CLIENT_COUNT];

// ESTA VARIABLE Y ESTRUCTURA LAS PUSE POR LAS FUNCIONES DE QUE ENVIAN EL REPORTE
/* Current report status*/
static struct keyboard_state {
	uint8_t ctrl_keys_state; /* Current keys state */
	uint8_t keys_state[KEY_PRESS_MAX];
} hid_keyboard_state;

/*---------------------------------------- HIDS FUNCTIONS AND CALLBACKS --------------------------------------------------------------------------------*/

static void hids_outp_rep_handler(struct bt_hids_rep *rep,
				  struct bt_conn *conn,
				  bool write)
{
	printk("%d - hids_outp_rep_handler\n",global_conter++); // FRANCISCO

	char addr[BT_ADDR_LE_STR_LEN];

	if (!write) {
		printk("Output report read\n");
		return;
	};

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	printk("Output report has been received %s\n", addr);
}


static void hids_boot_kb_outp_rep_handler(struct bt_hids_rep *rep,
					  struct bt_conn *conn,
					  bool write)
{

	printk("%d - hids_boot_kb_outp_rep_handler\n",global_conter++); // FRANCISCO

	char addr[BT_ADDR_LE_STR_LEN];

	if (!write) {
		printk("Output report read\n");
		return;
	};

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	printk("Boot Keyboard Output report has been received %s\n", addr);
}


static void hids_pm_evt_handler(enum bt_hids_pm_evt evt, 
                                struct bt_conn *conn)
{
	printk("%d - hids_pm_evt_handler\n",global_conter++); // FRANCISCO

	char addr[BT_ADDR_LE_STR_LEN];
	size_t i;

	for (i = 0; i < CONFIG_BT_HIDS_MAX_CLIENT_COUNT; i++) {
		if (conn_mode[i].conn == conn) {
			break;
		}
	}

	if (i >= CONFIG_BT_HIDS_MAX_CLIENT_COUNT) {
		printk("Cannot find connection handle when processing PM");
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	switch (evt) {
	case BT_HIDS_PM_EVT_BOOT_MODE_ENTERED:
		printk("Boot mode entered %s\n", addr);
		conn_mode[i].in_boot_mode = true;
		break;

	case BT_HIDS_PM_EVT_REPORT_MODE_ENTERED:
		printk("Report mode entered %s\n", addr);
		conn_mode[i].in_boot_mode = false;
		break;

	default:
		break;
	}
}


static void hid_init(void)
{
	int err;
	struct bt_hids_init_param    hids_init_obj = { 0 };
	struct bt_hids_inp_rep       *hids_inp_rep;
	struct bt_hids_outp_feat_rep *hids_outp_rep;

	printk("%d - hid_init\n",global_conter++);

	static const uint8_t report_map[] = {
		0x05, 0x01,       /* Usage Page (Generic Desktop) */
		0x09, 0x06,       /* Usage (Keyboard) */
		0xA1, 0x01,       /* Collection (Application) */

		/* Keys */
#if INPUT_REP_KEYS_REF_ID
		0x85, INPUT_REP_KEYS_REF_ID,
#endif
		0x05, 0x07,       /* Usage Page (Key Codes) */
		0x19, 0xe0,       /* Usage Minimum (224) */
		0x29, 0xe7,       /* Usage Maximum (231) */
		0x15, 0x00,       /* Logical Minimum (0) */
		0x25, 0x01,       /* Logical Maximum (1) */
		0x75, 0x01,       /* Report Size (1) */
		0x95, 0x08,       /* Report Count (8) */
		0x81, 0x02,       /* Input (Data, Variable, Absolute) */

		0x95, 0x01,       /* Report Count (1) */
		0x75, 0x08,       /* Report Size (8) */
		0x81, 0x01,       /* Input (Constant) reserved byte(1) */

		0x95, 0x06,       /* Report Count (6) */
		0x75, 0x08,       /* Report Size (8) */
		0x15, 0x00,       /* Logical Minimum (0) */
		0x25, 0x65,       /* Logical Maximum (101) */
		0x05, 0x07,       /* Usage Page (Key codes) */
		0x19, 0x00,       /* Usage Minimum (0) */
		0x29, 0x65,       /* Usage Maximum (101) */
		0x81, 0x00,       /* Input (Data, Array) Key array(6 bytes) */

		/* LED */
#if OUTPUT_REP_KEYS_REF_ID
		0x85, OUTPUT_REP_KEYS_REF_ID,
#endif
		0x95, 0x05,       /* Report Count (5) */
		0x75, 0x01,       /* Report Size (1) */
		0x05, 0x08,       /* Usage Page (Page# for LEDs) */
		0x19, 0x01,       /* Usage Minimum (1) */
		0x29, 0x05,       /* Usage Maximum (5) */
		0x91, 0x02,       /* Output (Data, Variable, Absolute), */
				  /* Led report */
		0x95, 0x01,       /* Report Count (1) */
		0x75, 0x03,       /* Report Size (3) */
		0x91, 0x01,       /* Output (Data, Variable, Absolute), */
				  /* Led report padding */

		0xC0              /* End Collection (Application) */
	};

	hids_init_obj.rep_map.data = report_map;
	hids_init_obj.rep_map.size = sizeof(report_map);

	hids_init_obj.info.bcd_hid = BASE_USB_HID_SPEC_VERSION;
	hids_init_obj.info.b_country_code = 0x00;
	hids_init_obj.info.flags = (BT_HIDS_REMOTE_WAKE |
				    BT_HIDS_NORMALLY_CONNECTABLE);

	hids_inp_rep =
		&hids_init_obj.inp_rep_group_init.reports[INPUT_REP_KEYS_IDX];
	hids_inp_rep->size = INPUT_REPORT_KEYS_MAX_LEN;
	hids_inp_rep->id = INPUT_REP_KEYS_REF_ID;
	hids_init_obj.inp_rep_group_init.cnt++;

	hids_outp_rep =
		&hids_init_obj.outp_rep_group_init.reports[OUTPUT_REP_KEYS_IDX];
	hids_outp_rep->size = OUTPUT_REPORT_MAX_LEN;
	hids_outp_rep->id = OUTPUT_REP_KEYS_REF_ID;
	hids_outp_rep->handler = hids_outp_rep_handler;									// ESTA ES DEL MISMO TIPO QUE LA DE LA LINEA 506
	hids_init_obj.outp_rep_group_init.cnt++;

	hids_init_obj.is_kb = true;
	hids_init_obj.boot_kb_outp_rep_handler = hids_boot_kb_outp_rep_handler;			// ESTA ES DEL MISMO TIPO QUE LA DE LA LINEA 502
	hids_init_obj.pm_evt_handler = hids_pm_evt_handler;

	err = bt_hids_init(&hids_obj, &hids_init_obj);
	__ASSERT(err == 0, "HIDS initialization failed\n");
}

/*---------------------------------------- FUNCTIONS -----------------------------------------------------------------------*/

static void advertising_start(void)
{
	int err;

	printk("%d - advertising_start\n",global_conter++); // FRANCISCO

	struct bt_le_adv_param *adv_param = BT_LE_ADV_PARAM(
						BT_LE_ADV_OPT_CONNECTABLE |
						BT_LE_ADV_OPT_ONE_TIME,
						BT_GAP_ADV_FAST_INT_MIN_2,
						BT_GAP_ADV_FAST_INT_MAX_2,
						NULL);

	err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), sd,
			      ARRAY_SIZE(sd));
	if (err) {
		if (err == -EALREADY) {
			printk("Advertising continued\n");
		} else {
			printk("Advertising failed to start (err %d)\n", err);
		}

		return;
	}

	is_adv = true;
	printk("Advertising successfully started\n");
}

static void num_comp_reply(bool accept)
{
	printk("%d - num_comp_reply\n",global_conter++); // FRANCISCO

	struct pairing_data_mitm pairing_data;
	struct bt_conn *conn;

	if (k_msgq_get(&mitm_queue, &pairing_data, K_NO_WAIT) != 0) {
		return;
	}

	conn = pairing_data.conn;

	if (accept) {
		bt_conn_auth_passkey_confirm(conn);
		printk("Numeric Match, conn %p\n", conn);
	} else {
		bt_conn_auth_cancel(conn);
		printk("Numeric Reject, conn %p\n", conn);
	}

	bt_conn_unref(pairing_data.conn);

	if (k_msgq_num_used_get(&mitm_queue)) {
		k_work_submit(&pairing_work);
	}
}

/*---------------------------------------- FUNCTIONES TO REPORTS-----------------------------------------------------------------------*/

bool get_BLE_state_conection()
{
	return state_conection;
}

bool get_state_adv()
{
	return is_adv;
}

bool get_BLE_notifi_error(void)
{
    return app_notifi_error_b;
}

bool string_send(char* cadena, int8_t len, string_complete_cb_t cb)
{
	if(ble_machine_state != BLE_WAITING_TO_SEND)	return false;
    if(cadena == NULL || len == 0 || cb == NULL)	return false;
	
	string_complete_cb 		= cb;
	sending_string_pointer 	= cadena;
	sending_string_len		= len;

	for (int8_t i = 0; i < len ; i++)
	{
		hid_kbd_state_key_set(sending_string_pointer[i]);
	}

	key_report_send();
	ble_machine_state = BLE_WAITING_SEND;
	return true;
}

void Report_complete_callback(struct bt_conn *conn, void *user_data) //Esta callback la tendria que pasar yo a la funcion de key_report_send
{ 
	printk("%d - Report_complete_callback  ",global_conter++); // FRANCISCO

	if(sending_string_pointer == NULL)
    {
		string_complete_cb();
		ble_machine_state 	= BLE_WAITING_TO_SEND;
	}
	else
	{
		for (int8_t i = 0 ; i < sending_string_len ; i++)
		{
			hid_kbd_state_key_clear(sending_string_pointer[i]);
		}
		sending_string_pointer = NULL;
		sending_string_len = 0;
		key_report_send();
		ble_machine_state = BLE_WAITING_SEND;
	}
}

int hid_kbd_state_key_set(uint8_t key)
{
	//printk("%d - key_set\n",global_conter++); // FRANCISCO

	uint8_t ctrl_mask = 0;//button_ctrl_code(key);

	if (ctrl_mask) {
		hid_keyboard_state.ctrl_keys_state |= ctrl_mask;
		return 0;
	}
	for (size_t i = 0; i < KEY_PRESS_MAX; ++i) {
		if (hid_keyboard_state.keys_state[i] == 0) {
			hid_keyboard_state.keys_state[i] = key;
			return 0;
		}
	}
	/* All slots busy */
	return -EBUSY;
}

int hid_kbd_state_key_clear(uint8_t key)
{
	//printk("%d - key_clear\n",global_conter++); // FRANCISCO

	uint8_t ctrl_mask = 0;//button_ctrl_code(key);

	if (ctrl_mask) {
		hid_keyboard_state.ctrl_keys_state &= ~ctrl_mask;
		return 0;
	}
	for (size_t i = 0; i < KEY_PRESS_MAX; ++i) {
		if (hid_keyboard_state.keys_state[i] == key) {
			hid_keyboard_state.keys_state[i] = 0;
			return 0;
		}
	}
	/* Key not found */
	return -EINVAL;
}

static int key_report_con_send(const struct keyboard_state *state,bool boot_mode,struct bt_conn *conn)
{

	//printk("%d - key_report_con_send\n",global_conter++); // FRANCISCO

	int err = 0;
	uint8_t  data[INPUT_REPORT_KEYS_MAX_LEN];
	uint8_t *key_data;
	const uint8_t *key_state;
	size_t n;

	data[0] = state->ctrl_keys_state;
	data[1] = 0;
	key_data = &data[2];
	key_state = state->keys_state;

	for (n = 0; n < KEY_PRESS_MAX; ++n) {
		*key_data++ = *key_state++;
	}
	if (boot_mode) {
		err = bt_hids_boot_kb_inp_rep_send(&hids_obj, conn, data,
							sizeof(data), NULL);
	} else {
		err = bt_hids_inp_rep_send(&hids_obj, conn,
						INPUT_REP_KEYS_IDX, data,
						sizeof(data), Report_complete_callback);
	}
	return err;
}

int key_report_send(void)
{

	printk("%d - key_report_send\n",global_conter++); // FRANCISCO

	for (size_t i = 0; i < CONFIG_BT_HIDS_MAX_CLIENT_COUNT; i++) {
		if (conn_mode[i].conn) {
			int err;

			err = key_report_con_send(&hid_keyboard_state,
						  conn_mode[i].in_boot_mode,
						  conn_mode[i].conn);
			if (err) {
				printk("Key report send error: %d\n", err);
				return err;
			}
		}
	}
	return 0;
}

/*---------------------------------------- AUTH CALLBACKS -------------------------------------------------------------------*/

void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{

	printk("%d - auth_passkey_display\n",global_conter++); // FRANCISCO

	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Passkey for %s: %06u\n", addr, passkey);
}

void auth_passkey_confirm(struct bt_conn *conn, unsigned int passkey)
{
	int err;

	printk("%d - auth_passkey_confirm\n",global_conter++); // FRANCISCO

	reconeccted_flag = false;

	struct pairing_data_mitm pairing_data;

	pairing_data.conn    = bt_conn_ref(conn);
	pairing_data.passkey = passkey;

	err = k_msgq_put(&mitm_queue, &pairing_data, K_NO_WAIT);
	if (err) {
		printk("Pairing queue is full. Purge previous data.\n");
	}

	/* In the case of multiple pairing requests, trigger
	 * pairing confirmation which needed user interaction only
	 * once to avoid display information about all devices at
	 * the same time. Passkey confirmation for next devices will
	 * be proccess from queue after handling the earlier ones.
	 */
	if (k_msgq_num_used_get(&mitm_queue) == 1) {
		k_work_submit(&pairing_work);
	}
}

void auth_cancel(struct bt_conn *conn)
{

	printk("%d - auth_cancel\n",global_conter++); // FRANCISCO

	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing cancelled: %s\n", addr);
}

/*---------------------------------------- PAIRING CALLBACKS -------------------------------------------------------------------*/

void pairing_complete(struct bt_conn *conn, bool bonded)
{

	printk("%d - pairing_complete\n",global_conter++); // FRANCISCO

	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing completed: %s, bonded: %d\n", addr, bonded);

	reconeccted_flag  = true;
    ble_machine_state = BLE_CONECCTED;

}

void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{

	printk("%d - pairing_failed\n",global_conter++); // FRANCISCO

	char addr[BT_ADDR_LE_STR_LEN];
	struct pairing_data_mitm pairing_data;

	if (k_msgq_peek(&mitm_queue, &pairing_data) != 0) {
		return;
	}

	if (pairing_data.conn == conn) {
		bt_conn_unref(pairing_data.conn);
		k_msgq_get(&mitm_queue, &pairing_data, K_NO_WAIT);
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing failed conn: %s, reason %d\n", addr, reason);
}

void pairing_process(struct k_work *work)
{
	int err;
	struct pairing_data_mitm pairing_data;

	printk("%d - pairing_process\n",global_conter++); // FRANCISCO

	char addr[BT_ADDR_LE_STR_LEN];

	err = k_msgq_peek(&mitm_queue, &pairing_data);
	if (err) {
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(pairing_data.conn),
			  addr, sizeof(addr));

	printk("Passkey for %s: %06u\n", addr, pairing_data.passkey);
	//printk("Press Button 1 to confirm, Button 2 to reject.\n");
    num_comp_reply(true); // CON ESTO HACEPTO LA CALVE DIRECTAMENTE
}

/*---------------------------------------- CONNECTION CALLBACKS -------------------------------------------------------------------*/

void connected(struct bt_conn *conn, uint8_t err)
{

	printk("%d - connected cb\n",global_conter++); // FRANCISCO

	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		printk("Failed to connect to %s (%u)\n", addr, err);
		return;
	}

	printk("Connected %s\n", addr);
	//dk_set_led_on(CON_STATUS_LED);

	err = bt_hids_connected(&hids_obj, conn);

	if (err) {
		printk("Failed to notify HID service about connection\n");
		return;
	}
	

	for (size_t i = 0; i < CONFIG_BT_HIDS_MAX_CLIENT_COUNT; i++) {
		if (!conn_mode[i].conn) {
			conn_mode[i].conn = conn;
			conn_mode[i].in_boot_mode = false;
			break;
		}
	}

	for (size_t i = 0; i < CONFIG_BT_HIDS_MAX_CLIENT_COUNT; i++) {
		if (!conn_mode[i].conn) {
			advertising_start();
			return;
		}
	}

	is_adv = false;
}

void disconnected(struct bt_conn *conn, uint8_t reason)
{
	int err;
	bool is_any_dev_connected = false;
	char addr[BT_ADDR_LE_STR_LEN];

	printk("%d - disconnected\n",global_conter++); // FRANCISCO
	state_conection = false;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected from %s (reason %u)\n", addr, reason);

	err = bt_hids_disconnected(&hids_obj, conn);

	if (err) {
		printk("Failed to notify HID service about disconnection\n");
	}

	for (size_t i = 0; i < CONFIG_BT_HIDS_MAX_CLIENT_COUNT; i++) {
		if (conn_mode[i].conn == conn) {
			conn_mode[i].conn = NULL;
		} else {
			if (conn_mode[i].conn) {
				is_any_dev_connected = true;
			}
		}
	}

	if (!is_any_dev_connected) {
		//dk_set_led_off(CON_STATUS_LED);
	}

	advertising_start();
	ble_machine_state = BLE_WAITING_CONECTION;
}

void security_changed(struct bt_conn *conn, bt_security_t level,enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	printk("%d - security_changed\n",global_conter++); // FRANCISCO

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		printk("Security changed: %s level %u\n", addr, level);
	} else {
		printk("Security failed: %s level %u err %d\n", addr, level,
			err);
	}

	if(reconeccted_flag) ble_machine_state = BLE_CONECCTED;
}

/*---------------------------------------- MACHINE --------------------------------------------------------------------------------*/

void ble_machine()
{
    uint8_t err;
    switch (ble_machine_state)
    {
        case BLE_CB_REGISTERS:
        {	
			err = bt_conn_auth_cb_register(&conn_auth_callbacks);
	        if (err) {
		        printk("BLE Failed to register authorization callbacks.\n");
				app_notifi_error_b = true;
		        break;
	        }

            err = bt_conn_auth_info_cb_register(&conn_auth_info_callbacks);
	        if (err) {
		        printk("BLE Failed to register authorization info callbacks.\n");
				app_notifi_error_b = true;
		        break;
	        }

			app_notifi_error_b = false;
            ble_machine_state  = BLE_INIT;
            break;
        }

        case BLE_INIT:
        {
            hid_init();

            err = bt_enable(NULL);
	        if (err) {
		        printk("BLE Bluetooth init failed (err %d)\n", err);
				app_notifi_error_b = true;
		        break;
	        }

			if (IS_ENABLED(CONFIG_SETTINGS)) {
		        settings_load();
	        }

			app_notifi_error_b = false;
            printk("BLE inicializado\n");
            ble_machine_state = BLE_ADVERTISING;
            break;
        }

        case BLE_ADVERTISING:
        {
			printk("BLE_ADVERTISING\n");

	        advertising_start();

	        k_work_init(&pairing_work, pairing_process);

            ble_machine_state = BLE_WAITING_CONECTION;
            break;
        }

        case BLE_WAITING_CONECTION:
        {
            break;
        }

        case BLE_CONECCTED:
        {
            printk("BLE coneccted and ready\n");
			state_conection = true;
			ble_machine_state = BLE_WAITING_TO_SEND;
            break;
        }

        case BLE_WAITING_TO_SEND:
        {

            break;
        }

        case BLE_WAITING_SEND:
        {

            break;
        }
		
    }
}