/* Thermostat Connecté
   Author: Amaury Graillat */

#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_wifi.h"
#include "esp_log.h"

#include "driver/i2c.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "TMP175\TMP75.h"

#include "wifi.h" 
#include "config.h"
#include "http.h"

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

    // Config I2C
    i2c_config_t i2cConfig = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = 21, // IO21
        .scl_io_num = 22, // IO22
        .sda_pullup_en = false,
        .scl_pullup_en = false,
        .master.clk_speed = 1000
    };

    const i2c_port_t i2c_port = 0;
    esp_err_t err = i2c_param_config(i2c_port /* I2C driver num 0 */, &i2cConfig);
    if(err) {
        printf("I2C init error.\n");
        return;
    }

    // Install driver
    i2c_driver_install(i2c_port, I2C_MODE_MASTER, 0, 0, ESP_INTR_FLAG_LEVEL1);
    
    tmp75_init(PREC_0_0625);

    float temp;
    tmp75_lect(&temp);

    while(true) {
        printf("%f\n", temp);
    }

    server = start_webserver();

    fflush(stdout);

    // tmp75_close();
    //esp_restart();
}

