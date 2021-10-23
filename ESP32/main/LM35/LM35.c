#include "LM35.h"
#include "driver/adc_common.h"
#include "hal/adc_types.h"
#include "esp_adc_cal.h"
#include "../config.h"

static adc1_channel_t adc_chan;
static esp_adc_cal_characteristics_t chars;

void LM35_init_adc1(adc1_channel_t adc_channel)
{
    adc_chan = adc_channel;

    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(adc_chan, ADC_ATTEN_0db );

    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_0, ADC_WIDTH_BIT_12, THERMOSTAT_ESP32_ADC_VREF, &chars);

}

double LM35_get_temp() 
{
    int read_raw = adc1_get_raw( adc_chan );

    // ADC: 12 bits 


    double val_mV = esp_adc_cal_raw_to_voltage(read_raw, &chars);

    double val_degrees = (val_mV / THERMOSTAT_LM35_MV_PER_DEGREES) - THERMOSTAT_LM35_DEGREES_OFFSET;

    return val_degrees;
}
