#ifndef UART_MACHINE_H
#define UART_MACHINE_H


/*---------------------------------------- INCLUDES --------------------------------------------------------------------------------*/

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <zephyr/sys/printk.h>
#include <zephyr/kernel.h>

#include <zephyr/drivers/uart.h>

/*---------------------------------------- DEFINITIONS --------------------------------------------------------------------------------*/

#define RECEIVE_BUFF_SIZE 	 8
#define RECEIVE_BUFF_NUMBER  32
#define RECEIVE_TIMEOUT_HAR  10000000//10000

#define K_INTERVAL_TO_REINIT 50

enum uart_machine
{
    NOT_USET,
    UART_INIT,
    UART_RX_INIT,
    UART_WORKING,
    UART_DEACTIVATE,
    WAIT_FOT_REINIT
};

uint8_t get_rdy_data();

void uart_machine();

bool get_UART_notifi_error(void);
bool get_UART_notifi_ready(void);

/*----------------------------------------------------------------------------------------------------------------------------------*/

#endif // UART_MACHINE_H
