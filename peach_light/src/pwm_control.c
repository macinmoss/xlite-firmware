

#include "pwm_control.h"
#include <zephyr/drivers/pwm.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>


static const struct pwm_dt_spec pwm_led0 = PWM_DT_SPEC_GET(DT_ALIAS(pwm_led0));
static const struct pwm_dt_spec pwm_led1 = PWM_DT_SPEC_GET(DT_ALIAS(pwm_led1));

#define MIN_PERIOD PWM_SEC(1U) / 128U
#define MAX_PERIOD PWM_SEC(1U)


//uint32_t pwmSetWarmLightDuty(uint32_t pwm);
//uint32_t pwmSetCoolLightDuty(uint32_t pwm);
//uint32_t getWarmLightBrightness(void);
//uint32_t getCoolLightBrightness(void);

//Local static variables: 
static uint32_t gCurrentWarmLightBrightness = 0;
static uint32_t gCurrentCoolLightBrightness = 75;



uint32_t max_period;
uint32_t period;
uint8_t dir = 0U;

	
// TODO: initializtion of pwm function needed 	
uint8_t init_pwm() {
	if (!device_is_ready(pwm_led0.dev) || !device_is_ready(pwm_led1.dev)) {
		return 0;
	}
	pm_device_runtime_enable(pwm_led0.dev);
	pm_device_runtime_enable(pwm_led1.dev);
	return 0;
}


uint32_t pwmSetWarmLightDuty(uint32_t pwm)
{
	pm_device_runtime_get(pwm_led0.dev);
	int ret = pwm_set_dt(&pwm_led0, MIN_PERIOD, (pwm * 40000));
	if (ret) {
		pm_device_runtime_put(pwm_led0.dev);
		return 0;
	}
	gCurrentWarmLightBrightness = pwm;
	return 0;
}

uint32_t pwmSetCoolLightDuty(uint32_t pwm)
{
	pm_device_runtime_get(pwm_led1.dev);
	int ret = pwm_set_dt(&pwm_led1, MIN_PERIOD, (pwm * 40000));
	if (ret) {
		pm_device_runtime_put(pwm_led1.dev);
		return 0;
	}
	gCurrentCoolLightBrightness = pwm;
	return 0;
}

uint32_t getWarmLightBrightness(void) {
	return gCurrentWarmLightBrightness;
}

uint32_t getCoolLightBrightness(void) {
	return gCurrentCoolLightBrightness;
}

/* Suspend PWM output without clearing stored brightness.
 * Used by the solar loop so the light restores when solar drops again. */
void suspendLight()
{
	pwm_set_dt(&pwm_led0, MIN_PERIOD, 0);
	pm_device_runtime_put(pwm_led0.dev);
	pwm_set_dt(&pwm_led1, MIN_PERIOD, 0);
	pm_device_runtime_put(pwm_led1.dev);
}

/* Full off — clears stored brightness (e.g. on first boot). */
void turnLightOff()
{
	suspendLight();
	gCurrentWarmLightBrightness = 0;
	gCurrentCoolLightBrightness = 0;
}