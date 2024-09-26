#ifndef APP_CONF_H
#define APP_CONF_H

// INCLUDES ------------------------------------------------------------------------------------------------------------------------

#include <zephyr/kernel.h>              // Este lo puse y creo que soluciono el error de inclucion de esta misma libreria en main.c
#include <zephyr/types.h>

#include <stddef.h>
#include <zephyr/sys/printk.h>
#include <string.h>
#include <zephyr/logging/log.h>

// DEFINITIONS ------------------------------------------------------------------------------------------------------------------------

#define LOG_LEVEL 3

#endif//APP_CONF_H