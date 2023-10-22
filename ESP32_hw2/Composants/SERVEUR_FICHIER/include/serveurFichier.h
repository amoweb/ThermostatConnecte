#ifndef SERVEURFICHIER_H
#define SERVEURFICHIER_H

#include "esp_err.h"

esp_err_t initServeurFichier(httpd_handle_t server, char *donnee_serveur);
esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filename);

#endif // SERVEURFICHIER_H