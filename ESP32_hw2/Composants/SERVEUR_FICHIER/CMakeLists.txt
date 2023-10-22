idf_component_register(SRCS "serveurFichier.c"
                    INCLUDE_DIRS "include"
                    EMBED_FILES "script.html"
                    REQUIRES vfs spiffs esp_http_server)
