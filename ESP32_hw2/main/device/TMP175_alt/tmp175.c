#include "driver/i2c.h"
#include "freertos/portmacro.h"

static const i2c_port_t i2c_port = 0;

void tmp175_alt_init()
{
    esp_err_t ret;

    // Config I2C
    i2c_config_t i2cConfig = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = 21, // IO21
        .scl_io_num = 22, // IO22
        .sda_pullup_en = false,
        .scl_pullup_en = false,
        .master.clk_speed = 2000
    };

    esp_err_t err = i2c_param_config(i2c_port /* I2C driver num 0 */, &i2cConfig);
    if(err) {
        printf("I2C init error.\n");
        return;
    }

    // Install driver
    i2c_driver_install(i2c_port, I2C_MODE_MASTER, 0, 0, ESP_INTR_FLAG_LEVEL1);
}

/*
   Returns temp in Celsius degrees from TMP175
   Note : resolution 1 Celsius degree
 */
static double preTmp175 = 0;
double tmp175_alt_get_temp() {

    esp_err_t ret;

    i2c_cmd_handle_t cmd_handle;

    ////////////// UPDATE CONFIGURATION //////////////////
    cmd_handle = i2c_cmd_link_create();
    i2c_master_start(cmd_handle);
    // Slave address
    i2c_master_write_byte(cmd_handle, /* TMP75 address */ (0x49<<1) | I2C_MASTER_WRITE, /* ack_en */ 1);
    // Pointer register: Configuration register (P1 = 0, P2 = 1)
    i2c_master_write_byte(cmd_handle, 1, /* ack_en */ 1);
    // Data byte 1
    i2c_master_write_byte(cmd_handle, 0b11100000 /* one shot, 12 bits */, /* ack_en */ 1);

    i2c_master_stop(cmd_handle);
    ret = i2c_master_cmd_begin(i2c_port, cmd_handle, 200);
    if(ret == ESP_FAIL) {
        printf("I2C #1 Error ACK not received.\n");
    } else if(ret != ESP_OK) {
        printf("I2C #1 Error 0x%x\n", ret);
    }
    i2c_cmd_link_delete(cmd_handle);

    vTaskDelay(230 / portTICK_PERIOD_MS);

    ////////////////////// READ TEMPERATURE /////////////////////
    uint8_t data[2] = {0, 0};

    cmd_handle = i2c_cmd_link_create();
    i2c_master_start(cmd_handle); 
    // Slave address
    i2c_master_write_byte(cmd_handle, /* TMP75 address */ (0x49<<1) | I2C_MASTER_WRITE, /* ack_en */ 1);
    // Pointer register: Temperature register (P1 = P2 = 0)
    i2c_master_write_byte(cmd_handle, 0, /* ack_en */ 1);
    
    // Slave address
    i2c_master_start(cmd_handle); 
    i2c_master_write_byte(cmd_handle, /* TMP75 address */ (0x49<<1) | I2C_MASTER_READ, /* ack_en */ 1);
    i2c_master_read(cmd_handle, data, /* data_len */ 2, /* ack_en */ 0);
    i2c_master_stop(cmd_handle);

    ret = i2c_master_cmd_begin(i2c_port, cmd_handle, 200);
    if(ret == ESP_FAIL) {
        printf("I2C #2 Error ACK not received.\n");
    } else if(ret != ESP_OK) {
        printf("I2C #2 Error 0x%x\n", ret);
    }
    i2c_cmd_link_delete(cmd_handle);

    double t = data[0] + (data[1] >> 4) * 0.0625;

    // Filter high values
    if(t > 40 || t < 0) {
        return preTmp175;
    }

    preTmp175 = t;

    return t;
}

void tmp175_alt_stop()
{
    i2c_driver_delete(i2c_port);
}
