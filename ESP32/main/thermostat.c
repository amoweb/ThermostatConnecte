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
        .master.clk_speed = 2000
    };

    const i2c_port_t i2c_port = 0;
    esp_err_t err = i2c_param_config(i2c_port /* I2C driver num 0 */, &i2cConfig);
    if(err) {
        printf("I2C init error.\n");
        return;
    }

    // Install driver
    i2c_driver_install(i2c_port, I2C_MODE_MASTER, 0, 0, ESP_INTR_FLAG_LEVEL1);

    uint8_t data[2] = {0, 0};

    ////////////// CONFIGURE TEMPERATURE RESOLUTION //////////////////
    i2c_cmd_handle_t cmd_handle = i2c_cmd_link_create();
    i2c_master_start(cmd_handle);
    // Slave address
    i2c_master_write_byte(cmd_handle, /* TMP75 address, write mode */ (0x49<<1) | 0, /* ack_en */ 1);
    // Pointer register: Configuration register (P1 = 0,  P2 = 1)
    i2c_master_write_byte(cmd_handle, 1, /* ack_en */ 1);
    // Resolution 0.125 (R1 = 1, R0 = 1)
    i2c_master_write_byte(cmd_handle, 0x60, /* ack_en */ 1);
    i2c_master_stop(cmd_handle);

    ret = i2c_master_cmd_begin(i2c_port, cmd_handle, 1000);
    if(ret == ESP_FAIL) {
        printf("I2C #0 Error ACK not received.\n");
        while(true)
            ;
    } else if(ret != ESP_OK) {
        printf("I2C #0 Error 0x%x\n", ret);
        while(true)
            ;
    }
    i2c_cmd_link_delete(cmd_handle);


    ////////////// SELECT TEMPERATURE REGISTER //////////////////
    // Slave address
    cmd_handle = i2c_cmd_link_create();
    i2c_master_start(cmd_handle);
    i2c_master_write_byte(cmd_handle, /* TMP75 address, write mode */ (0x49<<1) | 0, /* ack_en */ 1);
    // Pointer register: Temperature register (P1 = P2 = 0)
    i2c_master_write_byte(cmd_handle, 0, /* ack_en */ 0);
    i2c_master_stop(cmd_handle);
    ret = i2c_master_cmd_begin(i2c_port, cmd_handle, 1000);
    if(ret == ESP_FAIL) {
        printf("I2C #1 Error ACK not received.\n");
        while(true)
            ;
    } else if(ret != ESP_OK) {
        printf("I2C #1 Error 0x%x\n", ret);
        while(true)
            ;
    }
    i2c_cmd_link_delete(cmd_handle);

    while(true) {
        ////////////// READ TEMPERATURE //////////////////
        cmd_handle = i2c_cmd_link_create();
        i2c_master_start(cmd_handle);
        i2c_master_write_byte(cmd_handle, /* TMP75 address, read mode */ (0x49<<1) | 1, /* ack_en */ 1);
        i2c_master_read(cmd_handle, data, /* data_len */ 2, /* ack_en */ 1);
        i2c_master_stop(cmd_handle);
        ret = i2c_master_cmd_begin(i2c_port, cmd_handle, 1000);
        if(ret == ESP_FAIL) {
            printf("I2C #2 Error ACK not received.\n");
        } else if(ret != ESP_OK) {
            printf("I2C #2 Error 0x%x\n", ret);
        }
        i2c_cmd_link_delete(cmd_handle);

        printf("READ = %x %x\n", data[0], data[1]);
    }

    i2c_driver_delete(i2c_port);

    //tmp75_init(PREC_0_0625);

    // float temp;
    // while(true) {
    //     tmp75_lect(&temp);
    //     printf("%f\n", temp);
    // }

    server = start_webserver();

    fflush(stdout);

    // tmp75_close();
    //esp_restart();
}

