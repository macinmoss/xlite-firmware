#include "solar.h"
#include <nrfx_saadc.h>
#include <stdio.h> 
#include <stdint.h> 
#include "nrfx_errors.h"

//#include <nrfx_log.h>
#define ADC_REF_VOLTAGE_IN_MILLIVOLTS   600                                     /**< Reference voltage (in milli volts) used by ADC while doing conversion. */
#define ADC_PRE_SCALING_COMPENSATION    6                                       /**< The ADC is configured to use VDD with 1/3 prescaling as input. And hence the result of conversion is to be multiplied by 3 to get the actual value of the battery voltage.*/
#define DIODE_FWD_VOLT_DROP_MILLIVOLTS  270                                     /**< Typical forward voltage drop of the diode . */
#define ADC_RES_10BIT                   1024                                    /**< Maximum digital value for 10-bit ADC conversion. */
#define ADC_RESULT_IN_MILLI_VOLTS(ADC_VALUE) \
        ((((ADC_VALUE) *ADC_REF_VOLTAGE_IN_MILLIVOLTS) / ADC_RES_10BIT) * ADC_PRE_SCALING_COMPENSATION)

#define NRFX_SAADC_XLITE_CHANNEL_SE(_pin_p, _index)                  \
{                                                                      \
    .channel_config =                                                  \
    {                                                                  \
        NRFX_COND_CODE_1(NRF_SAADC_HAS_CH_CONFIG_RES,                  \
                         (.resistor_p = NRF_SAADC_RESISTOR_DISABLED,   \
                          .resistor_n = NRF_SAADC_RESISTOR_DISABLED,), \
                         ())                                           \
        .gain       = SAADC_CH_CONFIG_GAIN_Gain1_6,                                 \
        .reference  = NRF_SAADC_REFERENCE_INTERNAL,                    \
        .acq_time   = NRFX_SAADC_DEFAULT_ACQTIME,                      \
        NRFX_COND_CODE_1(NRF_SAADC_HAS_CONV_TIME,                      \
                         (.conv_time = NRFX_SAADC_DEFAULT_CONV_TIME,), \
                         ())                                           \
        .mode       = NRF_SAADC_MODE_SINGLE_ENDED,                     \
        .burst      = NRF_SAADC_BURST_DISABLED,                        \
    },                                                                 \
    .pin_p          = (nrf_saadc_input_t)_pin_p,                       \
    .pin_n          = NRF_SAADC_INPUT_DISABLED,                        \
    .channel_index  = _index,                                          \
}

nrfx_err_t err_code;
 
int16_t solar;
uint16_t battery;
static uint32_t gCurrentBattValue = 0;




static void handle_error(nrfx_err_t error_code)
{
    if (error_code!= NRFX_SUCCESS)
    {
        printf("ERROR");
        while(1){};
    }
}


uint8_t initSolarAdc() 
{

printf("entered solar ADC setup\n");


  nrfx_saadc_channel_t solarChannel = NRFX_SAADC_XLITE_CHANNEL_SE(NRF_SAADC_INPUT_AIN0, 0);
  //nrfx_saadc_channel_t batteryChannel = NRFX_SAADC_DEFAULT_CHANNEL_SE(NRF_SAADC_INPUT_VDD, 1);


 
    err_code = nrfx_saadc_init(NRFX_SAADC_DEFAULT_CONFIG_IRQ_PRIORITY);
    handle_error(err_code);
 
    err_code = nrfx_saadc_channels_config(&solarChannel, 1);
    handle_error(err_code);

 
    err_code = nrfx_saadc_simple_mode_set((1<<0),
                                          NRF_SAADC_RESOLUTION_10BIT,
                                          NRF_SAADC_OVERSAMPLE_DISABLED,
                                          NULL);
    handle_error(err_code);
 
    err_code = nrfx_saadc_buffer_set(&solar, 1);
    handle_error(err_code);  
 
    return 0;
}

uint8_t initBatteryAdc() 
{

printf("entered battery ADC setup\n");


  //nrfx_saadc_channel_t solarChannel = NRFX_SAADC_DEFAULT_CHANNEL_SE(NRF_SAADC_INPUT_AIN0, 0);
  nrfx_saadc_channel_t batteryChannel = NRFX_SAADC_XLITE_CHANNEL_SE(NRF_SAADC_INPUT_VDD, 0);


 
    err_code = nrfx_saadc_init(NRFX_SAADC_DEFAULT_CONFIG_IRQ_PRIORITY);
    handle_error(err_code);
 
    err_code = nrfx_saadc_channels_config(&batteryChannel, 1);
    handle_error(err_code);

 
    err_code = nrfx_saadc_simple_mode_set((1<<0),
                                          NRF_SAADC_RESOLUTION_10BIT,
                                          NRF_SAADC_OVERSAMPLE_DISABLED,
                                          NULL);
    handle_error(err_code);
 
    err_code = nrfx_saadc_buffer_set(&battery, 1);
    handle_error(err_code);  
 
    return 0;
}

uint32_t getSolarVolts()
{
    initSolarAdc();
    err_code = nrfx_saadc_mode_trigger();
    handle_error(err_code);
    int16_t result = solar;
    nrfx_saadc_uninit();
    return result;
}

uint32_t getBatteryVolts()
{
    initBatteryAdc();
    err_code = nrfx_saadc_mode_trigger();
    handle_error(err_code);
    uint32_t battery_lvl_in_milli_volts = ADC_RESULT_IN_MILLI_VOLTS(battery);
    gCurrentBattValue = battery_lvl_in_milli_volts;
    nrfx_saadc_uninit();
    return battery_lvl_in_milli_volts;
}

uint32_t rBattValue()
{
    return gCurrentBattValue;
}