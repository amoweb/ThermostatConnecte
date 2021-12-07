#pragma once

#include "driver/gpio.h"

void pushbutton_register_handler(gpio_num_t port, gpio_isr_t handler, void* args);

