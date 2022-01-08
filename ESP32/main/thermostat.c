/* Thermostat Connecté
Author: Amaury Graillat */

#include <time.h>
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

#include "esp_spiffs.h"

#include "device/LED/LED.h"
#include "device/relay/relay.h"
#include "device/LM35/LM35.h"
#include "device/TMP175_alt/tmp175.h"
#include "device/pushbutton/pushbutton.h"

#include "network/wifi/wifi.h" 
#include "network/http/http.h"

#include "controller/configuration/handlers.h"
#include "controller/hysteresis/hysteresis.h"

#include "config.h"

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
    relay_init(THERMOSTAT_RELAY_GPIO);

    server = start_webserver();

    register_get_endpoint(server, "/", http_get_handler);
    register_get_endpoint(server, "/temp", http_get_handler);
    register_post_endpoint(server, "/temp", http_post_handler_temperature);
    register_post_endpoint(server, "/time", http_post_handler_time_date);

    LM35_init_adc1(THERMOSTAT_LM35_ADC);

    pushbutton_register_handler(THERMOSTAT_PB_BLACK_GPIO, pushbutton_black_handler, NULL);
    pushbutton_register_handler(THERMOSTAT_PB_RED_GPIO, pushbutton_red_handler, NULL);

    hysteresis_init();
    hysteresis_set_target(23);


    printf("Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true
    };

    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            printf("Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            printf("Failed to find SPIFFS partition");
        } else {
            printf("Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    bool heat;
    while(true) {
        double tmp = tmp175_alt_get_temp();

        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        printf("%ld\n", ts.tv_sec);

        hysteresis_step(tmp, &heat);

        // led_set_level(THERMOSTAT_RELAY_GPIO, heat);

        printf("%f : %s\n", tmp, (heat?"HEAT":"NO"));
    }

    fflush(stdout);

    stop_webserver(server);
    tmp175_alt_stop();

    //esp_restart();
}

