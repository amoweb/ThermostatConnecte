#pragma once

void http_post_handler_temperature(const char* uri, const char* data);
void http_post_handler_time_date(const char* uri, const char* data);
void pushbutton_black_handler(void * args);
void pushbutton_red_handler(void * args);
const char* http_get_handler(const char* uri);

