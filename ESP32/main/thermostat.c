#include <stdint.h>
#include <stdio.h>
#include "sdkconfig.h"
#include "config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include <pthread.h>

#include "driver/spi_master.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_wifi.h"
#include "esp_log.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "esp_spiffs.h"

#include "device/LED/LED.h"
#include "device/relay/relay.h"
#include "device/ONE_WIRE/include/onewire.h"
#include "device/TMP175_alt/tmp175.h"
#include "device/pushbutton/pushbutton.h"
#include "device/BUS_SPI/include/bus_spi.h"
#include "device/RFM12/include/RFM12.h"

#include "network/wifi/wifi.h" 
#include "network/http/http.h"

#include "controller/configuration/storage.h"
#include "controller/configuration/handlers.h"
#include "controller/hysteresis/hysteresis.h"
#include "controller/estimator/estimator.h"

//////////////////////////
// Thermomètre sans fil
#define	LOCK_MUTEX_RFM12 pthread_mutex_lock(&E_R_RFM12->mutex);
#define	UNLOCK_MUTEX_RFM12 pthread_mutex_unlock(&E_R_RFM12->mutex);
// FREQ_BCLE assure le temps d'attente dans la boucle de mise à jour. A ajuster si modification du débit RFM12 ?
#define	FREQ_BCLE								20		// en ms = fréquence de scrutation des modifications html et RFM12
// FREQ_CAPT assure le temps de scrutation des capteurs internes
// la fréquence doit être supérieure à la fréquence de réception RFM de 4 secondes
#define	FREQ_CAPT								(300 * 1000) / FREQ_BCLE 	// en secondes
esp_err_t lect_DS18B20(float *temperature);

#define	LOCK_MUTEX_INTERFACE   pthread_mutex_lock(&mutexInterface); // INFOA("\t interface LOCK_MUTEX_INTERFACE");
#define	UNLOCK_MUTEX_INTERFACE pthread_mutex_unlock(&mutexInterface); // INFOA("\t interface UNLOCK_MUTEX_INTERFACE");
pthread_mutex_t mutexInterface;
EventGroupHandle_t main_event_group;	// groupe d'évènement du pgm principal
//////////////////////////


#ifdef CONFIG_CAPTEUR_DS18B20
/*==========================================================*/
/* lecture des données capteur DS18B20 */ /*
attendre a minima 30 s entre chaque mesure */
/*==========================================================*/
static float g_temperatureDS18B20 = 0.0f;

esp_err_t lect_DS18B20(float *temperature)
{
    esp_err_t err;

    err = ds18b20_measure(THERMOSTAT_DS18B20_GPIO, temperature);
    gpio_set_direction(THERMOSTAT_DS18B20_GPIO, GPIO_MODE_OUTPUT_OD);
    gpio_set_level(THERMOSTAT_DS18B20_GPIO, 1);
    if (err == ESP_OK) {
        LOCK_MUTEX_INTERFACE; /* On verrouille le mutex */
            g_temperatureDS18B20 = *temperature;
        UNLOCK_MUTEX_INTERFACE; /* On déverrouille le mutex */
        xEventGroupClearBits(main_event_group, ER_TEMP_INT);
    }
    else {
        xEventGroupSetBits(main_event_group, ER_TEMP_INT);
        printf("Erreur[%i] capteur interne DS18B20", err);
    }
    return	err;
}
#endif

/*==========================================================*/
/* gestionnaire pour mettre à jour le hardware */ /*  */
/*==========================================================*/
#define CMP_ID_RFM12(_id)		strncmp((char*)bufRX+RFM_ID, _id, 5) // retourne 0 si égal
void gestionMiseAJour()
{
    UBaseType_t uxPriority;
    uxPriority = uxTaskPriorityGet( NULL );
    printf("gestionMiseAJour - core = %d (priorite %d)", xPortGetCoreID(), uxPriority);
    int recu = VIDE;
    unsigned char  bufRX[SIZE_FIFO] = {0};
    _E_R_RFM12	*E_R_RFM12;
    static int tempo = 0;		// assure la surveillance de la présence réception de message par RFM12
    float temperature;
    float tempRFM12 = 0.0f;
#if CONFIG_TYPE_DHT
    float humidity;
#endif
    // 	int16_t raw;
    esp_err_t errCaptInt;
    esp_err_t	errRFM12;

    E_R_RFM12 = Init_RF12();

    // ============ Boucle principale ===============
    /* durée de la boucle ESP 32 : FREQ_BCLE ms
       surveille les entrées html ou RFM12
       lit les capteurs internes
       efectue la mise à jour de l'interface html et du hardware
     */

    while(1) {
        errRFM12 = ESP_OK;

        // ======== test si reception data RFM12
        // NOTE FREQ_BCLE assure le temps d'attente dans la boucle. A ajuster si modification du débit RFM12 ?
        if(recu != RECU) {		
            if(xQueueReceive(E_R_RFM12->IT_queue, &E_R_RFM12->octetRecu, FREQ_BCLE / portTICK_PERIOD_MS)) {
                recu = reception_RFM12(E_R_RFM12);
                // printf("RFM recu %i info %i oct 0x%02X", recu, E_R_RFM12->octetRecu);
            }
            else {
                E_R_RFM12->result = VIDE;
                recu = VIDE;				
                tempo++;
            }

            // ======== si réception RFM12 : mise à jour interface	
            if(recu == RECU) {
                stopIT_RFM12(E_R_RFM12);
                // copie les data
                for(int i=0; i<SIZE_FIFO; i++) {
                    bufRX[i] = E_R_RFM12->bufRX[i];
                }
                // teste ID du module pour éliminer les messages envoyés par d'autres modules
                // ======== module S_RFM_1 "SER11" : bouton + led
                if(CMP_ID_RFM12(S_RFM_1) == 0) {
                }
                // ======== module S_RFM_2	: DS18B20
                else if(CMP_ID_RFM12(S_RFM_2) == 0)	{
                    tempRFM12 = 0.0f;
                    tempRFM12 = ((bufRX[RFM_DATA+1] << 8) | bufRX[RFM_DATA]) / 16.0f;
                }
                else {
                    errRFM12 = ESP_FAIL;
                    printf("Reception RFM12 inconnu %02X %02X %02X %02X %02X", bufRX[1], bufRX[2], bufRX[3], bufRX[4], bufRX[5]);
                    // NOTE : délai pour limiter la réception à un seul message du même module 
                    // (les modules envoient une salve de trois messages)
                    vTaskDelay(300 / portTICK_PERIOD_MS);
                }

                // teste le premier octet des data envoyé par le module RFM12 pour vérifier son état
                if(errRFM12 == ESP_OK && bufRX[RFM_ETAT] != 0) 	/* si erreur */ {
                    errRFM12 = ESP_FAIL;
                    xEventGroupSetBits(main_event_group, ER_RFM12);
                    if(bufRX[RFM_ETAT] & DEFAUT_BAT) {
                        // NOTE le défaut batterie n'empêche pas d'avoir une valeur valide
                        errRFM12 = ESP_OK;
                        // 								LOCK_MUTEX_INTERFACE /* On verrouille le mutex */
                        // 						hardRFM12(premiere, bufRX);
                        // 								UNLOCK_MUTEX_INTERFACE;		/* On déverrouille le mutex */
                        printf("Erreur RFM12 %c%c%c%c%c Défaut batterie", bufRX[1], bufRX[2], bufRX[3], bufRX[4], bufRX[5]);
                    }
                    else if(bufRX[RFM_ETAT] & ERR_CKS) {
                        printf("Erreur RFM12 %c%c%c%c%c Défaut calcul checksum", bufRX[1], bufRX[2], bufRX[3], bufRX[4], bufRX[5]);
                    }
                    else if(bufRX[RFM_ETAT] & ERR_18B20) {
                        printf("Erreur RFM12 %c%c%c%c%c Défaut capteur DS18B20", bufRX[1], bufRX[2], bufRX[3], bufRX[4], bufRX[5]);
                    }
                }
                // message reçu par RFM12 OK
                if(errRFM12 == ESP_OK) {
                    LOCK_MUTEX_INTERFACE /* On verrouille le mutex */
                    rfm12_receive_handler(bufRX);
                    UNLOCK_MUTEX_INTERFACE;		/* On déverrouille le mutex */
                    xEventGroupClearBits(main_event_group, ER_RFM12);
                }
                E_R_RFM12->result = VIDE;
                recu = VIDE;

                // lecture capteurs internes après réception RFM12 DS18B20 et mise à jour "base de données"
                if(CMP_ID_RFM12(S_RFM_2) == 0)	{
                    tempo = 0;
#ifdef CONFIG_TYPE_DHT
                    errCaptInt = lect_dth(premiere, &temperature, &humidity);
                    if((errCaptInt == ESP_OK) & (errRFM12 == ESP_OK)) {
                        printf("ext\t%0.3f\tint\t%0.3f\t%0.3f", tempRFM12, temperature, humidity);
                    }
                    else if (errCaptInt == ESP_OK) {
                        printf("ext\terreur\tint\t%0.3f\t%0.3f", temperature, humidity);
                    }
                    else {
                        printf("ext\t%0.3f\tint\terreur", tempRFM12);
                    }
                    // NOTE : délai pour limiter la réception à un seul message du même module 
                    // (les modules envoient une salve de trois messages)
                    vTaskDelay(300 / portTICK_PERIOD_MS);
#endif
#ifdef CONFIG_CAPTEUR_DS18B20
                    // NOTE :  le délai de conversion du DS18B20 est de 750 ms
                    errCaptInt = lect_DS18B20(&temperature);
                    if((errCaptInt == ESP_OK) & (errRFM12 == ESP_OK)) {
                        printf("ext\t%0.3f\tint\t%0.3f", tempRFM12, temperature);
                    }
                    else if (errCaptInt == ESP_OK) {
                        printf("ext\terreur\tint\t%0.3f", temperature);
                    }
                    else {
                        printf("ext\t%0.3f\tint\terreur", tempRFM12);
                    }
#endif
                }	
                startIT_RFM12(E_R_RFM12);
            }
        }

        // ======== Si pas de reception RFM12 teste si modification via HTML
        // et lecture capteurs internes si perte de signal RFM 12
        if(recu == VIDE) {
            LOCK_MUTEX_INTERFACE /* On verrouille le mutex */
                // si modification via interface web met à jour la "base de données"
                //if(ptrEtatObj->obj_modif != NULL) {
                //    stopIT_RFM12(E_R_RFM12);
                //    ptrEtatObj->obj_modif->fonctionCtrl(ptrEtatObj->obj_modif, premiere);
                //    ptrEtatObj->obj_modif = NULL;
                //    startIT_RFM12(E_R_RFM12);
                //}
            UNLOCK_MUTEX_INTERFACE;		/* On déverrouille le mutex */

            // si perte de signal RFM 12
            // lit les valeurs des capteurs internes et met à jour la "base de données"
            // la fréquence de lecture des capteurs (FREQ_CAPT) doit être supérieure à la fréquence d'émission de RFM12 (4mn)
            if(tempo > FREQ_CAPT) {
                tempo = 0;
                stopIT_RFM12(E_R_RFM12);
#if CONFIG_TYPE_DHT
                errCaptInt = lect_dth(&temperature, &humidity);
                if(errCaptInt == ESP_OK) {
                    printf("ext\tperdu\tint\t%0.3f\t%0.3f", temperature, humidity);
                }
#endif
#if CONFIG_CAPTEUR_DS18B20
                errCaptInt = lect_DS18B20(&temperature);
                if(errCaptInt == ESP_OK) {
                    printf("ext\tperdu\tint\t%0.3f", temperature);
                }
#endif
                startIT_RFM12(E_R_RFM12);
                xEventGroupSetBits(main_event_group, ER_RFM12);
                // printf("perte signal RFM12");
            }
        }
    }
}


void init_gestionMiseAJour()
{
    main_event_group = xEventGroupCreate();
    xEventGroupClearBits(main_event_group, MAIN_OK | ER_RFM12 | ER_TEMP_INT);
    // ---------- INITIALISE thread de mise à jour
    // NOTE utiliser tskIDLE_PRIORITY pour éviter l'appel watchdog, mais utilise tout le temps processeur !
    xTaskCreatePinnedToCore(gestionMiseAJour, "threadMAJ", 10000, NULL, 1, NULL, 1);
}

void app_main(void)
{
    printf("Started.\n");

    static httpd_handle_t server;

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_init_sta();

    // ---------- INITIALISE bus SPI
#ifdef CONFIG_BUS_SPI
    int host_spi = SPI2_HOST; // HSPI
    init_SPI(CONFIG_GPIO_MOSI, CONFIG_GPIO_MISO, CONFIG_GPIO_SCLK, CONFIG_GPIO_CS, host_spi);
#endif

    ds18b20_init(THERMOSTAT_DS18B20_GPIO);

    //tmp175_alt_init();

    //led_init(THERMOSTAT_LED_GPIO);
    //relay_init(THERMOSTAT_RELAY_GPIO);

    server = start_webserver();

    register_get_endpoint(server, "/", http_get_handler);
    register_get_endpoint(server, "/temp", http_get_handler);
    register_get_endpoint(server, "/target", http_get_handler);
    register_get_endpoint(server, "/target_presence", http_get_handler);
    register_get_endpoint(server, "/target_absence", http_get_handler);
    register_get_endpoint(server, "/debug", http_get_handler);
    register_get_endpoint(server, "/stats", http_get_handler);
    register_get_endpoint(server, "/presence", http_get_handler);
    register_post_endpoint(server, "/heat", http_post_handler_heat);
    register_post_endpoint(server, "/target", http_post_handler_temperature);
    register_post_endpoint(server, "/time", http_post_handler_time_date);
    register_post_endpoint(server, "/presence", http_post_handler_presence);

    //LM35_init_adc1(THERMOSTAT_LM35_ADC);

    //pushbutton_register_handler(THERMOSTAT_PB_BLACK_GPIO, pushbutton_black_handler, NULL);
    //pushbutton_register_handler(THERMOSTAT_PB_RED_GPIO, pushbutton_red_handler, NULL);

    hysteresis_init();
    hysteresis_set_target(17);

    test_time();

    // Init time
    struct time initTime;
    initTime.day = 0;
    initTime.hour = 0;
    initTime.minute = 0;
    set_current_time(initTime);

    // Tests
    time_test();
    estimator_test();

    estimator_init();

    printf("Initializing SPIFFS\n");

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };

    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            printf("Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            printf("Failed to find SPIFFS partition");
        } else {
            printf("Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    init_gestionMiseAJour();

    init_presence_array();

    stats_record_s record;
    struct time t = get_current_time();
    bool preIsPresent = false;
    bool heat = false;
    while(true) {
        double temperaturePresence;
        double temperatureAbsence;
        get_temperature_target(&temperaturePresence, &temperatureAbsence);
        //double temperature = tmp175_alt_get_temp();

        // Thermomètre interne
        float tmpFloat = 0.0;

        lect_DS18B20(&tmpFloat);

        float tmpFloatOneWire = 0.0;
        ds18b20_measure(THERMOSTAT_DS18B20_GPIO, &tmpFloatOneWire);

        printf("tmpFloat = %f\n", tmpFloat);
        printf("tmpFloatOneWire = %f\n", tmpFloatOneWire);

        double temperature = (double)tmpFloat;

        hysteresis_step(temperature, &heat);
        //led_set_level(THERMOSTAT_RELAY_GPIO, heat);
        //led_set_level(THERMOSTAT_LED_GPIO, !heat); // false = on

        printf("%f : %s\n", temperature, (heat?"HEAT":"NO"));

        printf("Current time: %2d:%2d day=%d\n", t.hour, t.minute, t.day);

        estimator_step(temperature, heat, t);
        double slope = estimator_get_slope();

        printf("Slope: %.2f degrees/hour\n", slope);

        bool isPresent = presence_is_present(t);

        // Transition vers présence
        if(isPresent && !preIsPresent) {
            hysteresis_set_target(temperaturePresence);
        }

        // Transition vers absence
        if(!isPresent && preIsPresent) {
            hysteresis_set_target(temperatureAbsence);
        }

        struct time prochainDemarrage = presence_get_next_start(t);
        double tempsJusquAuProchainDemarrageHeures = time_duration_hour(t, prochainDemarrage);

        // Anticipe la chauffe
        if(temperature < temperaturePresence) {
            double dureeChauffeHeures = (temperaturePresence - temperature) / slope;
            if(!isPresent && tempsJusquAuProchainDemarrageHeures < dureeChauffeHeures) {
                hysteresis_set_target(temperaturePresence);
            }
            printf("tempsJusquAuProchainDemarrageHeures = %f\n", tempsJusquAuProchainDemarrageHeures);
            printf("dureeChauffeHeures = %f\n", dureeChauffeHeures);
        }

        printf("Next start: %2d:%2d day=%d\n", prochainDemarrage.hour, prochainDemarrage.minute, prochainDemarrage.day);

        // Enregistrer les statistiques
        record.time = t;
        record.temperature = temperature;
        record.targetTemperature = hysteresis_get_target();
        record.slope = slope;
        record.heat = heat;
        stats_add_record(record);

        preIsPresent = isPresent;
    }

    fflush(stdout);

    stop_webserver(server);
    //tmp175_alt_stop();

    //esp_restart();
}

