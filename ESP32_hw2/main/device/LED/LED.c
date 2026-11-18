#include "LED.h"
#include "driver/gpio.h"
#include "hal/gpio_types.h"

void led_init(gpio_num_t port)
{
    gpio_reset_pin(port);
    gpio_set_direction(port, GPIO_MODE_OUTPUT);
}

void led_set_level(gpio_num_t port, bool state)
{
    gpio_set_level(port, state);
}

void led_on(gpio_num_t port)
{
    gpio_set_level(port, 1);
}

void led_off(gpio_num_t port)
{
    gpio_set_level(port, 0);
}

