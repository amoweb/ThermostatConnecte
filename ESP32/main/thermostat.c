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

#include "TMP175\TMP75.h"

#include "wifi.h" 
#include "config.h"
#include "http.h"

#include "TMP175_alt/tmp175.h"

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

    server = start_webserver();

    register_get_endpoint(server, "/", http_get_handler);
    register_post_endpoint(server, "/echo", http_post_handler);

    while(true) {
        double tmp = tmp175_alt_get_temp();
        printf("%f\n", tmp);
    }

    fflush(stdout);

    stop_webserver(server);
    tmp175_alt_stop();

    //esp_restart();
}

