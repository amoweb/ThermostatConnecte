#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"

#include "data_uploader.h"

#include "config.h"

static const char *TAG = "Uploader";

#if CONFIG_DATA_UPLOADER_ENABLED

#include "esp_http_client.h"
#include "controller/configuration/storage.h"

#define STRING_SIZE (6+2+2+1 + 4) // "temperature,hour,minute,heat,"
void build_data_uploader_payload(char** payload, int* len)
{
    stats_record_s *part1;
    unsigned int sizePart1;
    stats_record_s *part2;
    unsigned int sizePart2;

    stats_get_all_records(&part1, &sizePart1, &part2, &sizePart2);

    *payload = NULL;
    *len = 0;

    if (sizePart1 + sizePart2 == 0)
    {
        return;
    }

    const unsigned int payload_len = 5 /*hist=*/ + (sizePart1 + sizePart2) * STRING_SIZE;
    char* str = malloc(payload_len + 1);
    if (str == NULL)
    {
        ESP_LOGE(TAG, "malloc failed");
        return;
    }

    *payload = str;

    unsigned int pos = 0;
    pos += snprintf(&str[pos], payload_len - pos, "hist=");
    for(int partNum = 0; partNum < 2; partNum++) {
        unsigned int sizePart = 0;
        stats_record_s* part = NULL;

        switch(partNum) {
            case 0:
                sizePart = sizePart1;
                part = part1;
                break;
            case 1:
                sizePart = sizePart2;
                part = part2;
                break;
        }

        for(int i=0; i<sizePart; i++) {
            stats_record_s* p = &part[i];
            const int written = snprintf(&str[pos], payload_len - pos, "%.2f,%d,%d,%d,", p->temperature,p->time.hour,p->time.minute,p->heat);
            if (written < 0 || written >= payload_len - pos)
            {
                *len = pos;
                return;
            }
            pos += (unsigned int)written;
        }
    }

    if (pos > 1 && str[pos-1] == ',')
    {
        pos--;
        str[pos] = 0;
    }

    *len = pos;
}

void upload_data(void)
{
    ESP_LOGD(TAG, "Upload data to %s.", CONFIG_DATA_UPLOADER_URL);

    esp_http_client_config_t config = {
        .url = CONFIG_DATA_UPLOADER_URL,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 2000,
    };

    // TODO : allocate a dynamic array with the post payload. Use 

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return;
    }

    esp_err_t err = esp_http_client_set_header(client, "X-API-Key", CONFIG_DATA_UPLOADER_API_KEY);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set X-API-Key header: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return;
    }

    err = esp_http_client_set_header(client, "Content-Type", "application/x-www-form-urlencoded");
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set Content-Type header: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return;
    }

    char* post_body = NULL;
    int body_len = 0;
    build_data_uploader_payload(&post_body, &body_len);

    ESP_LOGE(TAG, "payload=%s\n", post_body);

    err = esp_http_client_set_post_field(client, post_body, body_len);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set POST field: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        free(post_body); post_body = NULL;
        return;
    }

    // Blocking
    err = esp_http_client_perform(client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP POST failed: %s", esp_err_to_name(err));
    } else {
        int status_code = esp_http_client_get_status_code(client);
        int content_len = esp_http_client_get_content_length(client);
        ESP_LOGI(TAG, "HTTP POST status = %d, response length = %d", status_code, content_len);
    }

    esp_http_client_cleanup(client);
    free(post_body); post_body = NULL;
}

#else /* CONFIG_DATA_UPLOADER_ENABLED */

void upload_data(void)
{
    /* Data uploader is disabled in configuration. */
}

#endif /* CONFIG_DATA_UPLOADER_ENABLED */
