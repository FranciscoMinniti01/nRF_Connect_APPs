#ifndef UART_MACHINE_H
#define UART_MACHINE_H

// INCLUDES ------------------------------------------------------------------------------------------------------------------------

#include <zephyr/drivers/uart.h>

#include "app_config.h"

// DEFINITIONS ------------------------------------------------------------------------------------------------------------------------

#define K_INTERVAL_TO_REINIT 50

// VARIABLES ------------------------------------------------------------------------------------------------------------------------

typedef void (*Code_callback_t) (uint16_t index , uint16_t* buf , uint8_t len);

enum uart_machine
{
    NOT_USET,
    UART_INIT,
    UART_RX_INIT,
    UART_WORKING,
    UART_DEACTIVATE,
    WAIT_FOT_REINIT
};

// FUNCTIONS ------------------------------------------------------------------------------------------------------------------------

uint8_t get_rdy_data();
void uart_machine();
bool get_UART_notifi_error(void);
bool get_UART_notifi_ready(void);
// Analiza el buffer en busqueda del codigo, Si lo encuentra envia el largo del codigo y el indice a donde se encuentra.
void serch_code(Code_callback_t cb);

// ------------------------------------------------------------------------------------------------------------------------

#endif//UART_MACHINE_H
