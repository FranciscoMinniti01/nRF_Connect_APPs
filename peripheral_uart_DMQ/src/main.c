#include <zephyr/usb/usb_device.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <soc.h>
#include <zephyr/bluetooth/hci.h>

#include <zephyr/sys/printk.h>

#include "dmq_nus_service.h"                    // Incluye las funciones y definiciones particulares del servicio NUS utilizando el codigo modificado
#include "app_config.h"
LOG_MODULE_REGISTER(LOG_MAIN,LOG_MAIN_LEVEL);			                // Registro del modulo logger y configuracion del nivel
#include "uart_machine.h"
#include "ble_machine.h"

#include <dk_buttons_and_leds.h>
#define RUN_LED_BLINK_INTERVAL 1000
#define RUN_STATUS_LED DK_LED1
#define CON_STATUS_LED DK_LED2

enum app_machine
{
    APP_GET_UART,
    APP_SENT_BLE,
	APP_WAIT_BLE
};
static int8_t app_machine = APP_GET_UART;
#define BUFFER_LEN 6
static uint8_t buffer[BUFFER_LEN];
bool is_send = true;

static void bt_receive_cb(struct bt_conn *conn, const uint8_t *const data, uint16_t len)
{
	
}

void sent_cb(struct bt_conn *conn)
{
	is_send = true;
}

void send_enabled_cb(enum bt_nus_send_status status)
{

}

static struct bt_nus_cb nus_cb = {
	.received = bt_receive_cb,
	.sent = sent_cb,
	.send_enabled = send_enabled_cb
};

int main(void)
{
	LOG_INF("\r\n\n\n\nMAIN: Starting Bluetooth Peripheral UART\n");
	printk("\r\n\n\n\nMAIN: Starting Bluetooth Peripheral UART\n");

	static uint8_t data_len;
	uint8_t caracter;

	int blink_status = 0;
	int err = 0;

	err = dk_leds_init();
	if (err) {
		LOG_ERR("Cannot init LEDs (err: %d)", err);
	}

	err = bt_nus_init(&nus_cb);
	if (err) {
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
				if(caracter != 0)
				{
					data_len = 0;
					while (caracter != 0)
					{
						buffer[data_len] = caracter;
						data_len++;
						if(data_len == BUFFER_LEN) {
							caracter = 0;
						} else {
							caracter = get_rdy_data();
						}
					}
					app_machine = APP_SENT_BLE;
				} 
				break;

			case APP_SENT_BLE:
				if(get_BLE_state_conn())
				{
					if(!bt_nus_send(NULL,buffer,data_len))
					{
						is_send = false;
						app_machine = APP_WAIT_BLE;
					}
				}
				break;

			case APP_WAIT_BLE:
				if(is_send) app_machine = APP_GET_UART;
				else if(!get_BLE_state_conn()) is_send = true;
				break;
		}

		if(get_BLE_state_conn()) dk_set_led(CON_STATUS_LED, (++blink_status) % 2);
		else dk_set_led(CON_STATUS_LED, 0);

		dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);
		k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
	}
}
