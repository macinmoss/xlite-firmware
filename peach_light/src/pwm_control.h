#ifndef PWM_CONTROL_H__
#define PWM_CONTROL_H__
#include <stdint.h>

uint8_t init_pwm();
uint32_t pwmSetWarmLightDuty(uint32_t pwm);
uint32_t pwmSetCoolLightDuty(uint32_t pwm);
uint32_t getWarmLightBrightness(void);
uint32_t getCoolLightBrightness(void);
void suspendLight();
void turnLightOff();


#endif