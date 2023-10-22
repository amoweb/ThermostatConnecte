/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 zeroday nodemcu.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * -------------------------------------------------------------------------------
 * Portions copyright (C) 2000 Dallas Semiconductor Corporation, under the
 * following additional terms:
 *
 * Except as contained in this notice, the name of Dallas Semiconductor
 * shall not be used except as stated in the Dallas Semiconductor
 * Branding Policy.
 */

/**
 * @file onewire.c
 *
 * Routines to access devices using the Dallas Semiconductor 1-Wire(tm)
 * protocol.
 *
 * This is a port of a bit-banging one wire driver based on the implementation
 * from NodeMCU.
 *
 * This, in turn, appears to have been based on the PJRC Teensy driver
 * (https://www.pjrc.com/teensy/td_libs_OneWire.html), by Jim Studt, Paul
 * Stoffregen, and a host of others.
 *
 * The original code is licensed under the MIT license.  The CRC code was taken
 * (at least partially) from Dallas Semiconductor sample code, which was licensed
 * under an MIT license with an additional clause (prohibiting inappropriate use
 * of the Dallas Semiconductor name).  See the accompanying LICENSE file for
 * details.
 */

#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp32/rom/ets_sys.h>
#include "onewire.h"

#include "esp_log.h"

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*  constantes */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
static const char *TAG = "DS18B20";

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*  variables externes */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
// extern const char *type[];
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*  variables locales */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
static portMUX_TYPE mux_OW = portMUX_INITIALIZER_UNLOCKED;

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* defines */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
#define ONEWIRE_SKIP_ROM		0xcc
#define OW_RESET						480
#define OW_ATT_RESET_L			70
#define OW_ATT_RESET_H			230
#define OW_EMIS_0						65
#define OW_EMIS_1						10

#define PORT_ENTER_CRITICAL()	portENTER_CRITICAL(&mux_OW)
#define PORT_EXIT_CRITICAL()	portEXIT_CRITICAL(&mux_OW)

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*  fonctions locales */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*  fonctions globales */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

/*==========================================================*/
/*  */ /*  */
/*==========================================================*/

/*==========================================================*/
/* Waits up to `max_wait` microseconds for the specified pin to go high. */ /* 
Returns true if successful, false if the bus never comes high (likely shorted).
 */
/*==========================================================*/
esp_err_t _onewire_wait_for_bus(gpio_num_t pin, int attendu, int max_wait)
{
	for (int i = 0; i < ((max_wait + 4) / 5); i++) {
		if (gpio_get_level(pin) == attendu) {
	// Wait an extra 1us to make sure the devices have an adequate recovery time before we drive things low again.
			ets_delay_us(1);
			return ESP_OK;
		}
			ets_delay_us(5);
	}
	// Wait an extra 1us to make sure the devices have an adequate recovery time before we drive things low again.
	ets_delay_us(1);
	return ESP_ERR_TIMEOUT;
}

/*==========================================================*/
/* Perform the onewire reset function. */ /*
  We will wait up to 250uS for the bus to come high, if it doesn't then 
  it is broken or shorted and we return false;
  Returns true if a device asserted a presence pulse, false otherwise.
 */
/*==========================================================*/
bool onewire_reset(gpio_num_t pin)
{
	bool ret;
	gpio_set_pull_mode(pin, GPIO_FLOATING);
	gpio_set_direction(pin, GPIO_MODE_INPUT);

	gpio_set_level(pin, 1);
	// wait until the wire is high... just in case
	if (_onewire_wait_for_bus(pin, 1, 250) != ESP_OK) {
ESP_LOGW(TAG,"ERREUR %s debut", __FUNCTION__);
		return false;
	}

	// forcer le bus à '0'
	gpio_set_direction(pin, GPIO_MODE_OUTPUT_OD);
	gpio_set_level(pin, 0);
	ets_delay_us(OW_RESET);
	// libérer le bus
	PORT_ENTER_CRITICAL();
	gpio_set_direction(pin, GPIO_MODE_INPUT);
	gpio_set_level(pin, 1); // allow it to float
	// attente reset confirmé : '0' = OK
	ets_delay_us(OW_ATT_RESET_L);
	ret = !gpio_get_level(pin); // retourne '1' si reset confirmé par DS18B20
	PORT_EXIT_CRITICAL();
	
	// attente du retour à 1
	if (_onewire_wait_for_bus(pin, 1, 480 - OW_ATT_RESET_L) != ESP_OK) {
ESP_LOGW(TAG,"ERREUR %s fin", __FUNCTION__);
		return false;
	}

	return ret;
}

/*==========================================================*/
/* écrit un bit */ /*  */
/*==========================================================*/
esp_err_t _onewire_write_bit(gpio_num_t pin, bool v)
{
	// attente retour à 1 entre chaque bit
	if (_onewire_wait_for_bus(pin, 1, 10) != ESP_OK){
		return ESP_ERR_TIMEOUT;
	}
		
  gpio_set_direction(pin, GPIO_MODE_OUTPUT_OD);
	gpio_set_level(pin, 0);  // drive output low

	if (v)	// envoi '1'
	{
		ets_delay_us(OW_EMIS_1);
		gpio_set_level(pin, 1);  // allow output high
		ets_delay_us(65 - OW_EMIS_1);
	}
	else	// envoi '0'
	{
		ets_delay_us(OW_EMIS_0);
		gpio_set_level(pin, 1); // allow output high
	}
	ets_delay_us(1);
	gpio_set_direction(pin, GPIO_MODE_INPUT);
	gpio_set_level(pin, 1);  // allow output high

	return ESP_OK;
}

/*==========================================================*/
/* Write a byte. */ /* 
The writing code uses open-drain mode and expects the pullup
resistor to pull the line high when not driven low.  If you need strong
power after the write (e.g. DS18B20 in parasite power mode) then call
onewire_power() after this is complete to actively drive the line high.
 */
/*==========================================================*/
esp_err_t onewire_write(gpio_num_t pin, uint8_t v)
{
	for (uint8_t bitMask = 0x01; bitMask; bitMask <<= 1) {
		if (_onewire_write_bit(pin, (bitMask & v)) != ESP_OK) {
			return ESP_ERR_TIMEOUT;
		}
	}
	return ESP_OK;
}

/*==========================================================*/
/* lit un bit */ /*  */
/*==========================================================*/
int _onewire_read_bit(gpio_num_t pin)
{
	if (_onewire_wait_for_bus(pin, 1, 10) != ESP_OK) {
		return -1;
	}
  gpio_set_direction(pin, GPIO_MODE_OUTPUT_OD);
	gpio_set_level(pin, 0);
	ets_delay_us(2);
	gpio_set_level(pin, 1);  // let pin float, pull up will raise
	gpio_set_direction(pin, GPIO_MODE_INPUT);
	ets_delay_us(11);
	int r = gpio_get_level(pin);  // Must sample within 15us of start
	ets_delay_us(48);
	return r;
}

/*==========================================================*/
/* Read a byte */ /*  */
/*==========================================================*/
int onewire_read(gpio_num_t pin)
{
	int r = 0;
	for (uint8_t bitMask = 0x01; bitMask; bitMask <<= 1) {
		int bit = _onewire_read_bit(pin);
		if (bit < 0) {
			return -1;
		}
		else if (bit)
			r |= bitMask;
	}
	return r;
}

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* capteur one wire DS18B20 */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
#define ds18x20_CONVERT_T        0x44
#define ds18x20_READ_SCRATCHPAD  0xBE

/*==========================================================*/
/* Lecture température DS18B20 */ /* 
	retourne la valeur en degrés C dans temperature
	retourne ESP_FAIL si erreur
*/
/*==========================================================*/
esp_err_t ds18b20_measure(gpio_num_t pin, float *temperature)
{
	uint8_t scratchpad[8];
	int16_t temp;
	int ret;
	
	if (!onewire_reset(pin)) {
ESP_LOGW(TAG,"ERREUR reset %s", __FUNCTION__);
		return ESP_ERR_INVALID_RESPONSE;
	}
// ESP_LOGI(TAG,"reset OK %s", __FUNCTION__);

		PORT_ENTER_CRITICAL();
	if(onewire_write(pin, ONEWIRE_SKIP_ROM) != ESP_OK) {
		PORT_EXIT_CRITICAL();
ESP_LOGW(TAG,"ERREUR %s ONEWIRE_SKIP_ROM", __FUNCTION__);
		return ESP_FAIL;
	}
	// demande de lecture température
	if(onewire_write(pin, ds18x20_CONVERT_T) != ESP_OK) {
		PORT_EXIT_CRITICAL();
ESP_LOGW(TAG,"ERREUR %s ds18x20_CONVERT_T", __FUNCTION__);
		return ESP_FAIL;
	}
	PORT_EXIT_CRITICAL(); // OK
	
// ESP_LOGI(TAG,"ONEWIRE_SKIP_ROM - ds18x20_CONVERT_T OK %s", __FUNCTION__);

	// attente fin de conversion
	vTaskDelay(750 / portTICK_PERIOD_MS);
	
// ESP_LOGI(TAG,"fin de conversion OK %s", __FUNCTION__);

	if (!onewire_reset(pin)) {
ESP_LOGW(TAG,"ERREUR %s attente fin de conversion", __FUNCTION__);
		return ESP_ERR_INVALID_RESPONSE;
	}
// ESP_LOGI(TAG,"reset 2 OK %s", __FUNCTION__);

	PORT_ENTER_CRITICAL();
	if(onewire_write(pin, ONEWIRE_SKIP_ROM) != ESP_OK) {
		PORT_EXIT_CRITICAL();
ESP_LOGW(TAG,"ERREUR %s ONEWIRE_SKIP_ROM", __FUNCTION__);
		return ESP_FAIL;
	}
	if (onewire_write(pin, ds18x20_READ_SCRATCHPAD) != ESP_OK) {
		PORT_EXIT_CRITICAL();
ESP_LOGW(TAG,"ERREUR %s ds18x20_READ_SCRATCHPAD", __FUNCTION__);
		return ESP_FAIL;
	}

	for (int i = 0; i < 8; i++) {
		ret = onewire_read(pin);
		if(ret == -1) {
				PORT_EXIT_CRITICAL();
ESP_LOGW(TAG,"ERREUR %s onewire_read", __FUNCTION__);
			return ESP_FAIL;
		}
		scratchpad[i] = ret;
	}
	PORT_EXIT_CRITICAL(); // OK
			
	temp = scratchpad[1] << 8 | scratchpad[0];
	*temperature = ((float)temp * 625.0) / 10000;

// ESP_LOGI(TAG,"OK ds18x20 %0.3f" , *temperature);

	return ESP_OK;
}

