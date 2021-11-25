/* Thermostat Connecté
Author: Amaury Graillat */

#include <stdint.h>
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_wifi.h"
#include "esp_log.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "device/LED/LED.h"
#include "device/LM35/LM35.h"
#include "device/TMP175_alt/tmp175.h"
#include "device/pushbutton/pushbutton.h"

#include "network/wifi/wifi.h" 
#include "network/http/http.h"

#include "controller/hysteresis/hysteresis.h"

#include "config.h"

char str[50];
char* http_get_handler(const char* uri)
{
    double tmp = tmp175_alt_get_temp();
    sprintf(str, "%f\n", tmp);

    return str;
}

void http_post_handler(const char* uri, const char* data)
{
    printf("POST %s : %s\n", uri, data);
}

void pushbutton_black_handler(void * args)
{
    led_on(THERMOSTAT_LED_GPIO);
}

void pushbutton_red_handler(void * args)
{
    led_off(THERMOSTAT_LED_GPIO);
}

void app_main(void)
{
    printf("Started.\n");

    static httpd_handle_t server;

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_init_sta();

    tmp175_alt_init();

    led_init(THERMOSTAT_LED_GPIO);

    server = start_webserver();

    register_get_endpoint(server, "/temp", http_get_handler);
    register_post_endpoint(server, "/echo", http_post_handler);

    LM35_init_adc1(THERMOSTAT_LM35_ADC);

    pushbutton_register_handler(THERMOSTAT_PB_BLACK_GPIO, pushbutton_black_handler, NULL);
    pushbutton_register_handler(THERMOSTAT_PB_RED_GPIO, pushbutton_red_handler, NULL);

    hysteresis_init();
    hysteresis_set_target(23);

    bool heat;
    while(true) {
        double tmp = tmp175_alt_get_temp();

        hysteresis_step(tmp, &heat);

        printf("%f : %s\n", tmp, (heat?"HEAT":"NO"));
    }

    fflush(stdout);

    stop_webserver(server);
    tmp175_alt_stop();

    //esp_restart();
}

