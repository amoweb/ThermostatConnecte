#pragma once

#include "driver/adc_common.h"
#include <driver/adc.h>

void LM35_init_adc1(adc1_channel_t adc_channel);

double LM35_get_temp();

