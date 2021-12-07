#include "driver/gpio.h"

void pushbutton_register_handler(gpio_num_t port, gpio_isr_t handler, void* args)
{
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL<<port);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_POSEDGE;

    gpio_config(&io_conf);

    gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);
    gpio_isr_handler_add(port, handler, args);
}

