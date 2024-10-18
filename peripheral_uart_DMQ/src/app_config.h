#ifndef APP_CONFIG_H
#define APP_CONFIG_H

// INCLUDES ------------------------------------------------------------------------------------------------------------------------

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <zephyr/types.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
#define LOG_LEVEL 3

// BLE CONFIG ------------------------------------------------------------------------------------------------------------------------

// ADVERTISING
#define ADV_OPTIONS         BT_LE_ADV_OPT_CONNECTABLE      // Configuramos la publicidad como conectable
#define ADV_MIN_INTERVAL    BT_GAP_ADV_FAST_INT_MIN_2      // Configuramos el minimo intervalo de publicidad
#define ADV_MAX_INTERVAL    BT_GAP_ADV_FAST_INT_MAX_2      // Configuramos el maximo intervalo de publicidad
// ADVERTISING PACKET
#define ADV_PAC_FLAGS       BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR      // Publicidad conectable disponible durante un período de tiempo prolongado y no compatible con bluetooth clasico
#define DEVICE_NAME         CONFIG_BT_DEVICE_NAME   

// CONNECTION
#define CONN_MIN_INTERVAL   800
#define CONN_MAN_INTERVAL   800
#define CONN_LATENCY        10
#define CONN_TIMEOUT        400

// SECURITY
#define DEVICE_CODE         555555
#define PASSKEY             DEVICE_CODE

// NUS CONFIG ------------------------------------------------------------------------------------------------------------------------

#endif//APP_CONFIG_H