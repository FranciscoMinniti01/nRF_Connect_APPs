#ifndef APP_CONF_H
#define APP_CONF_H

#include <zephyr/kernel.h>              // Este lo puse y creo que soluciono el error de inclucion de esta misma libreria en main.c
#include <zephyr/types.h>

#include <stddef.h>
#include <zephyr/sys/printk.h>
#include <string.h>
#include <zephyr/logging/log.h>

#define LOG_NAME 	LOG
#define LOG_LEVEL 	3
LOG_MODULE_REGISTER(LOG_NAME,LOG_LEVEL);			// Registro del modulo logger y configuracion del nivel

#endif//APP_CONF_H