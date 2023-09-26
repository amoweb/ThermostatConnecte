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
 * @file onewire.h
 * @defgroup onewire onewire
 * @{
 *
 * @brief Routines to access devices using the Dallas Semiconductor 1-Wire(tm)
 *        protocol.
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
#ifndef __ONEWIRE_H__
#define __ONEWIRE_H__

#include <stdbool.h>
#include <stdint.h>
#include <driver/gpio.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief Perform a 1-Wire reset cycle.
 *
 * @param pin  The GPIO pin connected to the 1-Wire bus.
 *
 * @return `true` if at least one device responds with a presence pulse,
 *         `false` if no devices were detected (or the bus is shorted, etc)
 */
bool onewire_reset(gpio_num_t pin);

/**
 * @brief Write a byte on the onewire bus.
 *
 * The writing code uses open-drain mode and expects the pullup resistor to
 * pull the line high when not driven low. If you need strong power after the
 * write (e.g. DS18B20 in parasite power mode) then call ::onewire_power()
 * after this is complete to actively drive the line high.
 *
 * @param pin   The GPIO pin connected to the 1-Wire bus.
 * @param v     The byte value to write
 *
 * @return `true` if successful, `false` on error.
 */
esp_err_t onewire_write(gpio_num_t pin, uint8_t v);

/**
 * @brief Read a byte from a 1-Wire device.
 *
 * @param pin    The GPIO pin connected to the 1-Wire bus.
 *
 * @return the read byte on success, negative value on error.
 */
int onewire_read(gpio_num_t pin);

/* Lecture température DS18B20 
	retourne la valeur en degrés C dans temperature
	retourne ESP_FAIL si erreur
*/

esp_err_t ds18b20_measure(gpio_num_t pin, float *temperature);


#ifdef __cplusplus
}
#endif

/**@}*/

#endif  /* __ONEWIRE_H__ */
