#include <stdio.h>
#include <string.h>

#include "../../device/LED/LED.h"
#include "../../device/relay/relay.h"
#include "../../device/TMP175_alt/tmp175.h"

#include "../../controller/hysteresis/hysteresis.h"
#include "../../controller/configuration/storage.h"
#include "../../controller/estimator/estimator.h"

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

void http_post_handler_heat(const char* uri, const char* data)
{
    // tmptargetname=20
    printf("POST %s : %s\n", uri, data);

    double target_temperature = strtod(&data[14], NULL);

    printf("Chauffe immédiate : %f\n", target_temperature);

    hysteresis_set_target(target_temperature);
}

void http_post_handler_temperature(const char* uri, const char* data)
{
    printf("POST %s : %s\n", uri, data);

    // tmptargetpresencename=19&tmptargetabsencename=17

    char* p = NULL;
    double target_temperature_presence = strtod(&data[22], &p);
    double target_temperature_absence = strtod(&p[22], &p);

    printf("Consigne presence: %f\n", target_temperature_presence);
    printf("Consigne absence: %f\n", target_temperature_absence);

    set_temperature_target(target_temperature_presence, target_temperature_absence);
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
    led_set_level(THERMOSTAT_LED_GPIO, true); // off

    double temperatureAbsence;
    get_temperature_target(NULL, &temperatureAbsence);
    hysteresis_set_target(temperatureAbsence);
}

void pushbutton_red_handler(void * args)
{
    led_set_level(THERMOSTAT_LED_GPIO, false); // on

    double temperaturePresence;
    get_temperature_target(&temperaturePresence, NULL);
    hysteresis_set_target(temperaturePresence);
}

#define RESPONSE_BUFFER_SIZE (2*4096)
char str[RESPONSE_BUFFER_SIZE];
const char* http_get_handler(const char* uri)
{
    printf("Request page [%s]\n", uri);

    if(strcmp(uri, "/temp") == 0) {
        double tmp = tmp175_alt_get_temp();
        sprintf(str, "%.2f\n", tmp);

    } else if(strcmp(uri, "/target") == 0) {
        double tmp = hysteresis_get_target();
        sprintf(str, "%.2f\n", tmp);

    } else if(strcmp(uri, "/target_presence") == 0) {
        double presence = 0, absence = 0;
        get_temperature_target(&presence, &absence);
        sprintf(str, "%.2f\n", presence);

    } else if(strcmp(uri, "/target_absence") == 0) {
        double presence = 0, absence = 0;
        get_temperature_target(&presence, &absence);
        sprintf(str, "%.2f\n", absence);

    } else if(strcmp(uri, "/debug") == 0) {
        struct time t = get_current_time();
        struct time next_start = presence_get_next_start(t);
        double temperature = tmp175_alt_get_temp();
        stats_record_s r = stats_get_last_record();

        sprintf(str,
            "%s Temperature: %.2f\nTarget temperature: %.2f\n Slope: %.2f degrees/hour\nCurrent time: %2d:%2d day=%d\nNext start: %2d:%2d day=%d\n",
            r.heat?"(CHAUFFE)":"",
            temperature,
            r.targetTemperature,
            r.slope,
            t.hour, t.minute, t.day,
            next_start.hour, next_start.minute, next_start.day
        );
    } else if(strcmp(uri, "/stats") == 0) {
        stats_record_s *part1;
        unsigned int sizePart1;
        stats_record_s *part2;
        unsigned int sizePart2;

        stats_get_all_records(&part1, &sizePart1, &part2, &sizePart2);

        unsigned int pos = 0;
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
                pos += sprintf(&str[pos], "%.2f,%d,%d,%d,", p->temperature,p->time.hour,p->time.minute,p->heat);
                if(pos > RESPONSE_BUFFER_SIZE - 20) {
                    return str;
                }
            }
        }

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


