
// INCLUDES ------------------------------------------------------------------------------------------------------------------------

#include <zephyr/usb/usb_device.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <soc.h>
#include <zephyr/bluetooth/hci.h>

#include <dk_buttons_and_leds.h>

#include "dmq_nus_service.h"                    // Incluye las funciones y definiciones particulares del servicio NUS utilizando el codigo modificado
#include "app_config.h"
#include "uart_machine.h"
#include "ble_machine.h"

// DEFINITIONS ------------------------------------------------------------------------------------------------------------------------

#define RUN_LED_BLINK_INTERVAL 	1000
#define RUN_STATUS_LED 			DK_LED1
#define CON_STATUS_LED 			DK_LED2

#define BUFFER_LEN 				6

// VARIABLES ------------------------------------------------------------------------------------------------------------------------

LOG_MODULE_REGISTER(LOG_MAIN,LOG_MAIN_LEVEL);			                // Registro del modulo logger y configuracion del nivel

enum app_machine
{
    APP_GET_UART,
    APP_SENT_BLE,
	APP_WAIT_BLE
};

static int8_t app_machine = APP_GET_UART;

static uint8_t buffer[BUFFER_LEN];

bool is_nus_send = true;
bool nus_send_status;

// NUS FUNCTIONS ------------------------------------------------------------------------------------------------------------------------

static void bt_receive_cb(struct bt_conn *conn, const uint8_t *const data, uint16_t len)
{
	LOG_INF("RX: %s",data);
}

void sent_cb(struct bt_conn *conn) { is_nus_send = true; }

void send_enabled_cb(bool status) { nus_send_status = status; }

nus_cb_t nus_cb = {
	.received = bt_receive_cb,
	.sent = sent_cb,
	.send_enabled = send_enabled_cb
};

// MAIN FUNCTIONS ------------------------------------------------------------------------------------------------------------------------

int main(void)
{
	static uint8_t data_len;
	uint8_t caracter;
	int blink_status = 0;
	int err = 0;

	LOG_INF("\r\n\n\n\nMAIN: Starting Bluetooth Peripheral UART\n");

	if (dk_leds_init()) {
		LOG_ERR("Cannot init LEDs (err: %d)", err);
		return 0;
	}

	if (bt_nus_init(&nus_cb)) {
		LOG_ERR("Failed to initialize UART service (err: %d)", err);
		return 0;
	}

	for (;;)
	{
		ble_machine();

		uart_machine();

		switch (app_machine)
		{
			case APP_GET_UART:
				caracter = get_rdy_data();
				while (caracter != 0) {
					buffer[data_len] = caracter;
					data_len++;
					if(data_len == BUFFER_LEN) {
						caracter = 0;
						app_machine = APP_SENT_BLE;
					} else {
						caracter = get_rdy_data();
					}
				}
				break;

			case APP_SENT_BLE:
				if(get_BLE_state_conn()) {
					if(!bt_nus_send(NULL,buffer,data_len)) {
						is_nus_send = false;
						app_machine = APP_WAIT_BLE;
						data_len 	= 0;
					}
				}
				break;

			case APP_WAIT_BLE:
				if(is_nus_send) app_machine = APP_GET_UART;
				else if(!get_BLE_state_conn()) is_nus_send = true;
				break;
		}

		if(!is_nus_send) dk_set_led(CON_STATUS_LED, (++blink_status) % 2);
		else if(get_BLE_state_conn()) dk_set_led(CON_STATUS_LED, 1);
		else dk_set_led(CON_STATUS_LED, 0);

		dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);
		k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
	}
}

// ------------------------------------------------------------------------------------------------------------------------