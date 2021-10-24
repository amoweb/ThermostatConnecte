#pragma once

#include <esp_http_server.h>

httpd_handle_t start_webserver(void);

void register_get_endpoint(httpd_handle_t server, char* uri, char* (*fun_ptr)(const char*));

void register_post_endpoint(httpd_handle_t server, char* uri, void (*fun_ptr)(const char*, const char*));

void stop_webserver(httpd_handle_t server);

