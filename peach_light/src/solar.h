#ifndef SOLAR_H__
#define SOLAR_H__
#include <stdint.h>


uint8_t initSolarAdc();
uint8_t initBatteryAdc();
uint32_t getSolarVolts();
uint32_t getBatteryVolts();
uint32_t rBattValue();

#endif 