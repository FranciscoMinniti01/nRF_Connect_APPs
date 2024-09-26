#ifndef GPIOS_LEDS_H
#define GPIOS_LEDS_H


/*---------------------------------------- INCLUDES --------------------------------------------------------------------------------*/

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <errno.h>

/*----------------------------------------------------------------------------------------------------------------------------------*/

#define VISIBLE_PERIOD			PWM_HZ(5)		//FUNCIONA
#define NOT_VISIBLE_PERIOD		PWM_HZ(100)		//FUNCIONA con 100 y 200 y funciona para el pulso al dividirlo por 2,4 y 8

#define MIN_NOT_VISIBLE_PERIOD	PWM_HZ(50) // Este solo se usa en la funcion de calibracion del periodo la cual no uso por ahora

bool RGB_UI_init(void);
bool set_RGB(bool flicker, int8_t red, int8_t blue, int8_t green);

#endif // GPIOS_LEDS_H