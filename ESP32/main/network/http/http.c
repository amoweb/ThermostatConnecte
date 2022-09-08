#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include <esp_http_server.h>

static const char *TAG = "example";

#define MAX_REGISTERED_ENPOINT 10
#define MAX_RECV_POST_SIZE_BYTE 1024

// Store GET endpoints
unsigned int nb_registered_get_endpoint = 0;
struct get_endpoint {
    char* uri;
    const char* (*fun_ptr)(const char*);
};
static struct get_endpoint registered_function_get_endpoint[MAX_REGISTERED_ENPOINT];
httpd_uri_t registered_get_endpoint[MAX_REGISTERED_ENPOINT];

// Store POST endpoints
unsigned int nb_registered_post_endpoint = 0;
struct post_endpoint {
    char* uri;
    void (*fun_ptr)(const char*, const char*);
};
static struct post_endpoint registered_function_post_endpoint[MAX_REGISTERED_ENPOINT];
httpd_uri_t registered_post_endpoint[MAX_REGISTERED_ENPOINT];

/* An HTTP GET handler */
static esp_err_t http_get_handler(httpd_req_t *req)
{
    struct get_endpoint* f = (struct get_endpoint*)req->user_ctx;
    const char* resp_str = f->fun_ptr(f->uri);

    httpd_resp_send(req, resp_str, strlen(resp_str));

    /* After sending the HTTP response the old HTTP request
     * headers are lost. Check if HTTP request headers can be read now. */
    if (httpd_req_get_hdr_value_len(req, "Host") == 0) {
        ESP_LOGI(TAG, "Request headers lost");
    }
    return ESP_OK;
}

/**
  Register an HTTP get_endpoint.
  */
void register_get_endpoint(httpd_handle_t server, char* uri, const char* (*fun_ptr)(const char*))
{
    if(nb_registered_get_endpoint == MAX_REGISTERED_ENPOINT) {
        printf("Cannot register. Increase MAX_REGISTERED_ENPOINT.\n");
        return;
    }

    char* cpy_str = malloc(strlen(uri) + 1);
    strcpy(cpy_str, uri);
    registered_function_get_endpoint[nb_registered_get_endpoint].uri = cpy_str;
    registered_function_get_endpoint[nb_registered_get_endpoint].fun_ptr = fun_ptr;

    registered_get_endpoint[nb_registered_get_endpoint].uri       = cpy_str;
    registered_get_endpoint[nb_registered_get_endpoint].method    = HTTP_GET;
    registered_get_endpoint[nb_registered_get_endpoint].handler   = http_get_handler;
    // index in registered_function_array 
    registered_get_endpoint[nb_registered_get_endpoint].user_ctx  = (void*)(&registered_function_get_endpoint[nb_registered_get_endpoint]);

    httpd_register_uri_handler(server, &registered_get_endpoint[nb_registered_get_endpoint]);

    nb_registered_get_endpoint++;
}

/* An HTTP POST handler */
static esp_err_t http_post_handler(httpd_req_t *req)
{
    char fullBuf[MAX_RECV_POST_SIZE_BYTE];
    char buf[MAX_RECV_POST_SIZE_BYTE];
    int ret, remaining = req->content_len;

    if(remaining > MAX_RECV_POST_SIZE_BYTE) {
        return ESP_FAIL;
    }

    unsigned int pos = 0;
    while (remaining > 0) {
        /* Read the data for the request */
        if ((ret = httpd_req_recv(req, buf,
                        MIN(remaining, sizeof(buf)))) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                /* Retry receiving if timeout occurred */
                continue;
            }
            return ESP_FAIL;
        }

        memcpy(&fullBuf[pos], buf, ret);
        fullBuf[pos + ret] = 0;

        remaining -= ret;
        pos += ret;

        /* Log data received */
        ESP_LOGI(TAG, "=========== RECEIVED DATA ==========");
        ESP_LOGI(TAG, "%.*s", ret, buf);
        ESP_LOGI(TAG, "====================================");
    }

    struct post_endpoint* f = (struct post_endpoint*)req->user_ctx;
    f->fun_ptr(f->uri, fullBuf);

    // End response
    httpd_resp_set_status(req, "301 Moved Permanently");
    httpd_resp_set_hdr(req, "Location", "/");
    
    char* resp_str = "";
    httpd_resp_send(req, resp_str, strlen(resp_str));
    return ESP_OK;
}

/**
  Register an HTTP POST endpoint.
  */
void register_post_endpoint(httpd_handle_t server, char* uri, void (*fun_ptr)(const char*, const char*))
{
    if(nb_registered_post_endpoint == MAX_REGISTERED_ENPOINT) {
        printf("Cannot register. Increase MAX_REGISTERED_ENPOINT.\n");
        return;
    }

    char* cpy_str = malloc(strlen(uri) + 1);
    strcpy(cpy_str, uri);
    registered_function_post_endpoint[nb_registered_post_endpoint].uri = cpy_str;
    registered_function_post_endpoint[nb_registered_post_endpoint].fun_ptr = fun_ptr;

    registered_post_endpoint[nb_registered_post_endpoint].uri       = cpy_str;
    registered_post_endpoint[nb_registered_post_endpoint].method    = HTTP_POST;
    registered_post_endpoint[nb_registered_post_endpoint].handler   = http_post_handler;
    // index in registered_function_array 
    registered_post_endpoint[nb_registered_post_endpoint].user_ctx  = (void*)(&registered_function_post_endpoint[nb_registered_post_endpoint]);

    httpd_register_uri_handler(server, &registered_post_endpoint[nb_registered_post_endpoint]);

    nb_registered_post_endpoint++;
}


esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
    /* For any other URI send 404 and close socket */
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Not found");
    return ESP_FAIL;
}

httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

#if 0
    // TODO
    /* Register event handlers to stop the server when Wi-Fi is disconnected,
     * and re-start it upon connection.
     */
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));
#endif

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");

        // Test with curl -i -X POST -d '123' http://192.168.0.16/echo
        // httpd_register_uri_handler(server, &echo);

        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

void free_endpoints(httpd_handle_t server)
{
    for(int i = 0; i < nb_registered_get_endpoint; i++) {
        free(registered_function_get_endpoint[i].uri);
        registered_function_get_endpoint[i].uri = NULL;
    }

    for(int i = 0; i < nb_registered_post_endpoint; i++) {
        free(registered_function_post_endpoint[i].uri);
        registered_function_post_endpoint[i].uri = NULL;
    }
}

void stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    httpd_stop(server);
    free_endpoints(server);
}

static void disconnect_handler(void* arg, esp_event_base_t event_base, 
                               int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server) {
        ESP_LOGI(TAG, "Stopping webserver");
        stop_webserver(*server);
        *server = NULL;
    }
}

static void connect_handler(void* arg, esp_event_base_t event_base, 
                            int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server == NULL) {
        ESP_LOGI(TAG, "Starting webserver");
        *server = start_webserver();
    }
}

