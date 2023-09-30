#ifndef THERMOSTAT_CONFIG
#define THERMOSTAT_CONFIG

/* This uses WiFi configuration that you can set via project configuration menu

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define THERMOSTAT_ESP_WIFI_SSID "mywifissid"
*/
#define THERMOSTAT_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define THERMOSTAT_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define THERMOSTAT_ESP_MAXIMUM_RETRY  CONFIG_ESP_MAXIMUM_RETRY

#define THERMOSTAT_LED_GPIO 9 // IO9
#define THERMOSTAT_RELAY_GPIO 14 // IO14

#define THERMOSTAT_PB_BLACK_GPIO 19 // IO19
#define THERMOSTAT_PB_RED_GPIO 23 // IO23

//#define THERMOSTAT_LM35_ADC ADC1_CHANNEL_1 // IO37
//#define THERMOSTAT_LM35_MV_PER_DEGREES 10 // LM35: 10 mV / °C
//#define THERMOSTAT_LM35_DEGREES_OFFSET -3

#define THERMOSTAT_DS18B20_GPIO GPIO_NUM_32

#define THERMOSTAT_ESP32_ADC_VREF 1100

#define CONFIG_CAPTEUR_DS18B20 1
#define CONFIG_F_RFM_433 1
#define CONFIG_TYPE_RFM12 1

#define MAIN_OK     BIT0
#define ER_RFM12    BIT1
#define ER_TEMP_INT BIT2

#endif
