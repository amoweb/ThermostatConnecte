#ifndef THERMOSTAT_CONFIG
#define THERMOSTAT_CONFIG

/* This uses WiFi configuration that you can set via project configuration menu

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define THERMOSTAT_ESP_WIFI_SSID "mywifissid"
*/
#define THERMOSTAT_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define THERMOSTAT_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define THERMOSTAT_ESP_MAXIMUM_RETRY  CONFIG_ESP_MAXIMUM_RETRY

#endif
