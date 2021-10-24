#pragma once

#include "driver/gpio.h"

void led_init(gpio_num_t port);

void led_on(gpio_num_t port);

void led_off(gpio_num_t port);

