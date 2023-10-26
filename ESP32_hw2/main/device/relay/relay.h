#pragma once

#include "driver/gpio.h"

void relay_init(gpio_num_t port);

void relay_on(gpio_num_t port);

void relay_off(gpio_num_t port);

void relay_set_level(gpio_num_t port, bool state);
