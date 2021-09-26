#pragma once

#include <esp_http_server.h>

httpd_handle_t start_webserver(void);

void register_endpoint(char* uri, char* (*fun_ptr)());

void stop_webserver(httpd_handle_t server);
