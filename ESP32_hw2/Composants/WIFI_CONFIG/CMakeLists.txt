idf_component_register(SRCS "WIFI_CONFIG.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_wifi nvs_flash json driver esp_http_server app_update
                    EMBED_TXTFILES update.html )
