#ifndef APP_CONFIG_H
#define APP_CONFIG_H

// INCLUDES ------------------------------------------------------------------------------------------------------------------------

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <zephyr/types.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>

// APP CONFIG ------------------------------------------------------------------------------------------------------------------------

// LOG_ERR 1 - LOG_WRN 2 - LOG_INF 3 - LOG_DBG 4
#define LOG_MAIN_LEVEL      3
#define LOG_BLE_LEVEL       3
#define LOG_UART_LEVEL      3
#define LOG_NUS_LEVEL       3

// BLE CONFIG ------------------------------------------------------------------------------------------------------------------------

// ADVERTISING
#define ADV_OPTIONS         BT_LE_ADV_OPT_CONNECTABLE      // Configuramos la publicidad como conectable
#define ADV_MIN_INTERVAL    BT_GAP_ADV_FAST_INT_MIN_2      // Configuramos el minimo intervalo de publicidad
#define ADV_MAX_INTERVAL    BT_GAP_ADV_FAST_INT_MAX_2      // Configuramos el maximo intervalo de publicidad
#define ADV_ADDR_DIREC      NULL                            // address of peer for directed advertising
// ADVERTISING PACKET
#define ADV_FLAGS           BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR      // Publicidad conectable disponible durante un período de tiempo prolongado y no compatible con bluetooth clasico
#define DEVICE_NAME         CONFIG_BT_DEVICE_NAME  
#define ADV_UUID_SERVICE    BT_UUID_NUS_VAL

// CONNECTION
#define CONN_MIN_INTERVAL   800
#define CONN_MAN_INTERVAL   800
#define CONN_LATENCY        10
#define CONN_TIMEOUT        400

// SECURITY
#define DEVICE_CODE         555555
#define PASSKEY             DEVICE_CODE

// NUS CONFIG ------------------------------------------------------------------------------------------------------------------------



// UART CONFIG ------------------------------------------------------------------------------------------------------------------------

#include <zephyr/drivers/uart.h>

#define UART_BAUDRATE           9600
#define UART_PARITY             UART_CFG_PARITY_NONE
#define UART_STOP_BITS          UART_CFG_STOP_BITS_1
#define UART_DATA_BITS          UART_CFG_DATA_BITS_8
#define UART_FLOW_CTRL          UART_CFG_FLOW_CTRL_NONE

#define RECEIVE_BUFF_SIZE 	    8
#define RECEIVE_BUFF_NUMBER     32
#define RECEIVE_TIMEOUT_HAR_US  10000000

#define CODE                    "DMQ"
#define LEN_CODE                3

// ------------------------------------------------------------------------------------------------------------------------

#endif//APP_CONFIG_H