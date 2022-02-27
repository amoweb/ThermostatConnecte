#include "relay.h"
#include "driver/gpio.h"
#include "hal/gpio_types.h"

void relay_init(gpio_num_t port)
{
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL<<port);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;

    gpio_config(&io_conf);
}

void relay_set_level(gpio_num_t port, bool state)
{
    gpio_set_level(port, state);
}

void relay_on(gpio_num_t port)
{
    gpio_set_level(port, 1);
}

void relay_off(gpio_num_t port)
{
    gpio_set_level(port, 0);
}

