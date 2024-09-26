// INCLUDES ------------------------------------------------------------------------------------------------------------------------ 

#include <errno.h>
#include <zephyr/sys/byteorder.h>
//#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <soc.h>
#include <assert.h>
#include <zephyr/spinlock.h>

#include "APP_conf.h"
#include "ble_machine.h"
#include "uart_machine.h"
#include "gpios_leds.h"

// VARIABLES ------------------------------------------------------------------------------------------------------------------------

#define K_INTERVAL  100//1							// Tiempo

#define KEY_CODE_BUFFER_LEN 6 						// Largo del buffer que toma de UART y envia por BLE (6 es el valor maximo y es el maximo numero de teclas simultaneas que se pueden presionar KEY_PRESS_MAX en ble_machine.c )
uint8_t key_code_buffer[KEY_CODE_BUFFER_LEN];

bool is_send = true;								// Bandera para verificar la confirmacion del envio de los datos

enum app_machine
{
    APP_GET_UART,
    APP_SENT_BLE,
	APP_WAIT_BLE
};

enum UI_RGB_state_machine
{
    INIT,
    BLE_ADV,
	UART_NOT_READY,
	READY,
	TRANSMITINEDO,
	BLE_ERROR,
	UART_ERROR,
	NOT_STATE,
};

int8_t app_machine = APP_GET_UART;
int8_t RGB_machine = INIT;

// KEYCODE ------------------------------------------------------------------------------------------------------------------------

// Array para mapear los códigos de teclado inglés
static uint8_t codigosTeclado[255] = {
	['a'] = 0x04, ['b'] = 0x05, ['c'] = 0x06, ['d'] = 0x07, ['e'] = 0x08,
	['f'] = 0x09, ['g'] = 0x0a, ['h'] = 0x0b, ['i'] = 0x0c, ['j'] = 0x0d,
	['k'] = 0x0e, ['l'] = 0x0f, ['m'] = 0x10, ['n'] = 0x11, ['o'] = 0x12,
	['p'] = 0x13, ['q'] = 0x14, ['r'] = 0x15, ['s'] = 0x16, ['t'] = 0x17,
	['u'] = 0x18, ['v'] = 0x19, ['w'] = 0x1a, ['x'] = 0x1b, ['y'] = 0x1c,
	['z'] = 0x1d, ['0'] = 0x27, ['1'] = 0x1e, ['2'] = 0x1f, ['3'] = 0x20,
	['4'] = 0x21, ['5'] = 0x22, ['6'] = 0x23, ['7'] = 0x24, ['8'] = 0x25,
	['9'] = 0x26, [13 ] = 0x28, [' '] = 0x2c, ['.'] = 0x37, [','] = 0x36,
	[10]  = 0x11
};

// FUNCTIONS ------------------------------------------------------------------------------------------------------------------------

int obtenerCodigoTeclado(char caracter)
{
    int ascii = (int)caracter;
	int temp;

    if (ascii >= 0 && ascii <= 254) {
        temp = codigosTeclado[ascii];
		return temp==0 ? 0x2c : temp;
    } else {
		LOG_ERR("No se encontro el codigo del caracter %c ", caracter);
        return 0x2c;
    }
}

void string_complete_app_cb()
{
	is_send = true;
}

void UI_RGB_machine(void)
{
	static int8_t old_RGB_machine = -1;

	// Serie de if que permiten determinar el estado del dispositivo
	if(get_BLE_notifi_error()) RGB_machine = BLE_ERROR;
	else if(get_UART_notifi_error()) RGB_machine = UART_ERROR;
	else if(!is_send) RGB_machine = TRANSMITINEDO;
	else if(get_BLE_state_conection())
	{
		if(get_UART_notifi_ready()) RGB_machine = READY;
		else RGB_machine = UART_NOT_READY;
	}
	else if(get_state_adv()) RGB_machine = BLE_ADV;
	else if(RGB_machine == INIT);
	else RGB_machine = NOT_STATE;

	if(old_RGB_machine == RGB_machine) return;
	old_RGB_machine = RGB_machine;

	switch (RGB_machine)
	{
		case INIT:
			set_RGB(false,-1,1,-1); //AZUL PRENDIDO
			break;
		case BLE_ERROR:
			set_RGB(true,1,-1,-1);  //ROJO PARPADIANDO
			break;
		case UART_ERROR:
			set_RGB(true,1,-1,1);   //AMARILLO PARPADIANDO
			break;
		case TRANSMITINEDO:
			set_RGB(true,-1,-1,1);  //VERDE PARPADIANDO
			break;
		case READY:
			set_RGB(false,-1,-1,1); //VERDE PRENDIDO
			break;
		case UART_NOT_READY:
			set_RGB(false,1,-1,1);  //AMARILLO PRENDIDO
			break;
		case BLE_ADV:
			set_RGB(true,-1,1,-1);  //AZUL PARPADIANDO
			break;
		case NOT_STATE:
		default:
			set_RGB(true,1,1,1);  	//BLANCO PARPADIANDO
			break;
	}
}

// MAIN ------------------------------------------------------------------------------------------------------------------------

int main(void)
{

	LOG_INF("\r\n\n\n\n MAIN: Starting Bluetooth Peripheral HIDS keyboard example\n");

	static uint8_t data_len;
	uint8_t caracter;

	if(!RGB_UI_init()){
		LOG_ERR("Error: can not init RGB UI");
		return 0;
	}

	RGB_machine = INIT;   // ESTE ESTADO SE TENDRIA QUE PONER TAMBIEN SI LAS MAQUINAS DE ESTADO SE REINICIAN //FRAN

	for (;;)
	{
		UI_RGB_machine();

		ble_machine();

		uart_machine();

		switch (app_machine)
		{
			case APP_GET_UART:
				//ACA QUIZAS DEBERIA VERIFICAR SI HAY CONECCION BLE ANTES DE EMPEZAR A PEDIR PAQUETES A LA UART //FRAN
				caracter = get_rdy_data();
				if(caracter != 0)
				{
					data_len = 0;
					while (caracter != 0)
					{
						key_code_buffer[data_len] = obtenerCodigoTeclado(caracter);
						data_len++;
						if(data_len == KEY_CODE_BUFFER_LEN) {
							caracter = 0;
						} else {
							caracter = get_rdy_data();
						}
					}
					app_machine = APP_SENT_BLE;
				} 
				break;

			case APP_SENT_BLE:
				if(get_BLE_state_conection())
				{
					if(string_send(key_code_buffer,data_len,string_complete_app_cb))
					{
						is_send = false;
						app_machine = APP_WAIT_BLE;
					}
				}
				break;

			case APP_WAIT_BLE:
				if(is_send) app_machine = APP_GET_UART;
				else if(!get_BLE_state_conection()) is_send = true;
				break;
		
		}
		
		k_sleep(K_MSEC(K_INTERVAL));

	}
}
