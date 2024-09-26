/*---------------------------------------- INCLUDES --------------------------------------------------------------------------------*/

#include "gpios_leds.h"

/*---------------------------------------- VARIABLES --------------------------------------------------------------------------------*/
static const struct pwm_dt_spec pwm_ledred   = PWM_DT_SPEC_GET(DT_NODELABEL(pwm_ledred));
static const struct pwm_dt_spec pwm_ledblue  = PWM_DT_SPEC_GET(DT_NODELABEL(pwm_ledblue));
static const struct pwm_dt_spec pwm_ledgreen = PWM_DT_SPEC_GET(DT_NODELABEL(pwm_ledgreen));

/*bool config_period_channel(const struct pwm_dt_spec *dev ,uint32_t* period)//, uint32_t* pulse)
{
	uint32_t reduced_period;
	bool last_test = false;

	printk("Calibrating for channel %d...\n", dev->channel);

	while(pwm_set(dev, dev->channel, *period, *period, dev->flags))
	{
		if(last_test = true) return false;

		reduced_period = *period - (*period / 4);

		if (reduced_period < MIN_NOT_VISIBLE_PERIOD)
		{
			*period = MIN_NOT_VISIBLE_PERIOD;
			last_test = true;
    	}
		else
		{
        	*period = reduced_period;
    	}
	}
	printk("Done calibrating\n");
	return true;
}*/

bool set_RGB(bool flicker, int8_t red, int8_t blue, int8_t green)
{
	//	>0 Prendido al 100
	//	0 prendido al 50
	//	<0 apagado

	uint8_t error;
	uint32_t perido=80;
	uint8_t divider;

	error = pwm_set(pwm_ledred.dev, pwm_ledred.channel, perido, 	0, pwm_ledred.flags);
	if(error){
		if(error) printk("Error: can not clear led %s",pwm_ledred.dev->name);
		return false;	
	}
	error = pwm_set(pwm_ledblue.dev, pwm_ledblue.channel, perido, 	0, pwm_ledblue.flags);
	if(error){
		if(error) printk("Error: can not clear led %s",pwm_ledblue.dev->name);
		return false;	
	}
	error = pwm_set(pwm_ledgreen.dev, pwm_ledgreen.channel, perido, 0, pwm_ledgreen.flags);
	if(error){
		if(error) printk("Error: can not clear led %s",pwm_ledgreen.dev->name);
		return false;	
	}

	if(flicker){
		perido = VISIBLE_PERIOD;
		divider = 2U;
	}else{
		perido = NOT_VISIBLE_PERIOD;
		divider = 1U;
	}

	if(red>0)  		error = pwm_set(pwm_ledred.dev, pwm_ledred.channel, perido, perido/divider	, pwm_ledred.flags);
	else if(red<0)	error = pwm_set(pwm_ledred.dev, pwm_ledred.channel, perido, 0	 	 		, pwm_ledred.flags);
	else if(red==0)	error = pwm_set(pwm_ledred.dev, pwm_ledred.channel, perido, perido/(2U*divider)		, pwm_ledred.flags);
	if(error){
		if(error) printk("Error: can not sed led %s",pwm_ledred.dev->name);
		return false;	
	}

	if(blue>0)  	error = pwm_set(pwm_ledblue.dev, pwm_ledblue.channel, perido, perido/divider	, pwm_ledblue.flags);
	else if(blue<0)	error = pwm_set(pwm_ledblue.dev, pwm_ledblue.channel, perido, 0	 	 			, pwm_ledblue.flags);
	else if(blue==0)	error = pwm_set(pwm_ledblue.dev, pwm_ledblue.channel, perido, perido/(2U*divider)			, pwm_ledblue.flags);
	if(error){
		if(error) printk("Error: can not sed led %s",pwm_ledblue.dev->name);
		return false;	
	}

	if(green>0)  	  error = pwm_set(pwm_ledgreen.dev, pwm_ledgreen.channel, perido, perido/divider	, pwm_ledgreen.flags);
	else if(green<0)  error = pwm_set(pwm_ledgreen.dev, pwm_ledgreen.channel, perido, 0	 	 			, pwm_ledgreen.flags);
	else if(green==0)  error = pwm_set(pwm_ledgreen.dev, pwm_ledgreen.channel, perido, perido/(2U*divider)			, pwm_ledgreen.flags);
	if(error){
		printk("Error: can not sed led %s",pwm_ledgreen.dev->name);
		return false;	
	}

	return true;
}

bool RGB_UI_init(void)
{
	if (!pwm_is_ready_dt(&pwm_ledred)) {
		printk("Error: PWM device %s is not ready\n",pwm_ledred.dev->name);
		return false;
	}
	if (!pwm_is_ready_dt(&pwm_ledblue)) {
		printk("Error: PWM device %s is not ready\n",pwm_ledblue.dev->name);
		return false;
	}
	if (!pwm_is_ready_dt(&pwm_ledgreen)) {
		printk("Error: PWM device %s is not ready\n",pwm_ledgreen.dev->name);
		return false;
	}

    if(!set_RGB(false,-1,-1,-1)){
        printk("Error: can not set init RGB state");
        return false;
    }

	return true;
}