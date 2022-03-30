#include <stdio.h>
#include <string.h>

#include "../../device/LED/LED.h"
#include "../../device/relay/relay.h"
#include "../../device/TMP175_alt/tmp175.h"

#include "../../controller/hysteresis/hysteresis.h"
#include "../../controller/configuration/storage.h"

#include "../../config.h"

void http_post_handler_time_date(const char* uri, const char* data)
{
    printf("POST %s : %s\n", uri, data);

    struct time t;

    const char* string = strstr(data, "day=");

    sscanf(string, "day=%d&hour=%d&minute=%d", &t.day, &t.hour, &t.minute);

    printf("SET hour=%d, minute=%d, day=%d\n", t.hour, t.minute, t.day);

    set_current_time(t);
}

void http_post_handler_temperature(const char* uri, const char* data)
{
    printf("POST %s : %s\n", uri, data);

    double target_temperature = strtod(&data[14], NULL);

    printf("Target temperature : %f\n", target_temperature);

    hysteresis_set_target(target_temperature);
}

void http_post_handler_presence(const char* uri, const char* data)
{
    printf("POST %s : %s\n", uri, data);

    set_presence_array_from_string(data);
    print_presence_array();
    // h0a=7&m0a=00&h0b=8&m0b=30&h0c=18&m0c=00&h0d=22&m0d=00&h1a=7&m0a=00&h1b=8&m0b=30&h1c=18&m0c=00&h1d=22&m0d=00&h2a=7&m0a=00&h2b=8&m0b=30&h2c=18&m0c=00&h2d=22&m0d=00&h3a=7&m0a=00&h3b=8&m0b=30&h3c=18&m0c=00&h3d=22&m0d=00&h4a=7&m0a=00&h4b=8&m0b=30&h4c=18&m0c=00&h4d=22&m0d=00&h5a=7&m0a=00&h5b=8&m0b=30&h5c=18&m0c=00&h5d=22&m0d=00&h6a=7&m0a=00&h6b=8&m0b=30&h6c=18&m0c=00&h6d=22&m0d=00
}

void pushbutton_black_handler(void * args)
{
    led_on(THERMOSTAT_LED_GPIO);
    // relay_on(THERMOSTAT_RELAY_GPIO);
}

void pushbutton_red_handler(void * args)
{
    led_off(THERMOSTAT_LED_GPIO);
    // relay_off(THERMOSTAT_RELAY_GPIO);
}

#define RESPONSE_BUFFER_SIZE (2*4096)
char str[RESPONSE_BUFFER_SIZE];
const char* http_get_handler(const char* uri)
{
    printf("Request page [%s]\n", uri);

    if(strcmp(uri, "/temp") == 0) {
        double tmp = tmp175_alt_get_temp();
        sprintf(str, "%f\n", tmp);

    } else if(strcmp(uri, "/target") == 0) {
        double tmp = hysteresis_get_target();
        sprintf(str, "%f\n", tmp);

    } else if(strcmp(uri, "/") == 0) {

        // TODO on pourrait optimiser ici en évitant de tout charger en mémoire
        // avant d'envoyer. Mais plutôt créer un handler pour le GET qui fait
        // les lectures et les sends en même temps.

        // Open for reading hello.txt
        FILE* f = fopen("/spiffs/index.html", "r");

        if (f == NULL) {
            return "ERROR failed to open file.";
        }

        // Read file
        const unsigned int chunkSizeBytes = 32;
        unsigned int readSize = 0;
        char* buffer = str;
        do {
            readSize = fread(buffer, 1, chunkSizeBytes, f);

            if(buffer - str + chunkSizeBytes > RESPONSE_BUFFER_SIZE) {
                printf("File too long.");
                break;
            }

            buffer = buffer + chunkSizeBytes;

        } while(readSize == chunkSizeBytes);

        *buffer = 0;

        fclose(f);

        return str;
    }

    return str;
}


