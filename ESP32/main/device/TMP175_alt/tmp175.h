#ifndef TMP175_alt_h
#define TMP175_alt_h

#include "driver/i2c.h"

void tmp175_alt_init();

/*
   Returns temp in Celsius degrees from TMP175
Note : resolution 1 Celsius degree
 */
double tmp175_alt_get_temp();

void tmp175_alt_stop();

#endif

