// Maj 2023-06

/* 		Configuration esp-idf : */ /*
	- valider WiFi NVS flash (CONFIG_ESP32_WIFI_NVS_ENABLED) pour permettre le stockage des paramètres de config wifi en NVS
	- Utiliser une table de partition customisée, 
			- inclure une partition NVS pour la sauvergarde de la config wifi
			- inclure les partition OTA pour la mise à jour du firmware
*/

/* 		Modification config WIFI ou mise à jour firmware	*/ /*
		Dans les 10 secondes suivant le reset appuyer sur le bouton "boot" pour
		passer en mode "point d'accès" pour pouvoir configurer le WIFI ou
		effectuer une mise à jour du firmware

		Enregistrement connection wifi SSID et mot de passe */ /*
	- se connecter au reseau wifi "ESP32", mot de passe "serveuresp"
	- dans un navigateur web aller à la page 198.168.4.1
	- entrer le nom et mot de passe du réseau auquel se connecter
			=> l'ESP32 se connectera au reseau lors des prochains démarrages
REMARQUE : la modification des paramètres d'accès au réseau wifi ne fonctionnent qu'en se connectant au wifi ESP32

		Mise à jour du firmware */ /*
	- se connecter au reseau wifi "ESP32", mot de passe "serveuresp"
	- dans un navigateur web aller à la page 198.168.4.1/update
*/


/* TODO */ /*
	vérifier la reconnection suite à perte du wifi
*/

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include <pthread.h>
#include "esp_system.h"
#include "esp_mac.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include <esp_http_server.h>
#include <esp_wifi.h>
#include "esp_spiffs.h"
#include <sys/param.h>
#include <esp_ota_ops.h>
#include "driver/spi_master.h"

#include "maison.h"
#include "WIFI_CONFIG.h"
#include "HEURE_SNTP.h"

#include "bus_spi.h"
#include "RFM12.h"
#include "onewire.h"

#include "network/http/http.h"

#include "controller/configuration/storage.h"
#include "controller/configuration/handlers.h"
#include "controller/hysteresis/hysteresis.h"
#include "controller/estimator/estimator.h"

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* defines */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
#define	LOCK_MUTEX_INTERFACE   pthread_mutex_lock(&mutexInterface);
#define	UNLOCK_MUTEX_INTERFACE pthread_mutex_unlock(&mutexInterface);
#define	LOCK_MUTEX_RFM12   pthread_mutex_lock(&E_R_RFM12->mutex);
#define	UNLOCK_MUTEX_RFM12 pthread_mutex_unlock(&E_R_RFM12->mutex);
// FREQ_BCLE assure le temps d'attente dans la boucle de mise à jour. A ajuster si modification du débit RFM12 ?
#define	FREQ_BCLE								20		// en ms = fréquence de scrutation des modifications html et RFM12
// FREQ_CAPT assure le temps de scrutation des capteurs internes
// la fréquence doit être supérieure à la fréquence de réception RFM de 4 secondes
#define	FREQ_CAPT								(300 * 1000) / FREQ_BCLE 	// en secondes


#if (CONFIG_TYPE_DHT)
#include <dht.h>
#if defined(CONFIG_TYPE_DHT11)
#define SENSOR_TYPE DHT_TYPE_DHT11
#warning "CONFIG_TYPE_DHT11"
#endif
#if defined(CONFIG_TYPE_AM2301)
#define SENSOR_TYPE DHT_TYPE_AM2301
#warning "CONFIG_TYPE_AM2301"
#endif
#if defined(CONFIG_TYPE_SI7021)
#define SENSOR_TYPE DHT_TYPE_SI7021
#warning "CONFIG_TYPE_SI7021"
#endif
#elif (CONFIG_CAPTEUR_DS18B20)
#include <onewire.h>	
#warning "CONFIG_CAPTEUR_DS18B20"
#endif

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*  constantes */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
static const char *TAG = "Maison";

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*  variables externes */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
pthread_mutex_t mutexInterface;

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*  variables locales */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
EventGroupHandle_t main_event_group;	// groupe d'évènement du pgm principal

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*  fonctions globales */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*  fonctions locales */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

#if CONFIG_TYPE_DHT
/*==========================================================*/
/* lecture des données capteur DTH */ /*
NOTE : If you read the sensor data too often, it will heat up
http://www.kandrsmith.org/RJS/Misc/Hygrometers/dht_sht_how_fast.html
attendre a minima 30 s entre chaque mesure
                                       */
/*==========================================================*/
esp_err_t lect_dth(void *premiere, float *temperature, float *humidity) {
    esp_err_t err;

    err = dht_read_float_data(SENSOR_TYPE, CONFIG_GPIO_ONE_WIRE, humidity, temperature);
    if (err == ESP_OK) {
        LOCK_MUTEX_INTERFACE 			/* On verrouille le mutex */
            printf("lect dth\n");
        //hardDHT(premiere, *temperature, *humidity, err);
        UNLOCK_MUTEX_INTERFACE;		/* On déverrouille le mutex */
        xEventGroupClearBits(main_event_group, ER_TEMP_INT);
        // LOGI("DHT11 : %0.1f °C - %0.1f %%", temperature, humidity);
    }
    else {
        xEventGroupSetBits(main_event_group, ER_TEMP_INT);
        printf("Erreur[%i] capteur interne DHT11", err);
    }
    return err;
}
#endif

#if CONFIG_CAPTEUR_DS18B20
/*==========================================================*/
/* lecture des données capteur DS18B20 */ /*
                                             attendre a minima 30 s entre chaque mesure
                                           */
/*==========================================================*/
esp_err_t lect_DS18B20(void *premiere, float *temperature) {
    esp_err_t err;

    err = ds18b20_measure(CONFIG_GPIO_ONE_WIRE, temperature);
    gpio_set_direction(CONFIG_GPIO_ONE_WIRE, GPIO_MODE_OUTPUT_OD);
    gpio_set_level(CONFIG_GPIO_ONE_WIRE, 1);
    if (err == ESP_OK) {
        LOCK_MUTEX_INTERFACE 			/* On verrouille le mutex */
            //hardDS18B20(premiere, *temperature);
            printf("temperature = %f\n", *temperature);
        UNLOCK_MUTEX_INTERFACE;		/* On déverrouille le mutex */
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
void gestionMiseAJour(void *premiere) {
    UBaseType_t uxPriority;
    uxPriority = uxTaskPriorityGet( NULL );
    ESP_LOGW(TAG, "gestionMiseAJour - core = %d (priorite %d)", xPortGetCoreID(), uxPriority);
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

    while(1) {	
        errRFM12 = ESP_OK;

        // ======== test si reception data RFM12
        // NOTE FREQ_BCLE assure le temps d'attente dans la boucle. A ajuster si modification du débit RFM12 ?
        if(recu != RECU) {
            if(xQueueReceive(E_R_RFM12->IT_queue, &E_R_RFM12->octetRecu, FREQ_BCLE / portTICK_PERIOD_MS)) {
                recu = reception_RFM12(E_R_RFM12);
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
                    LOCK_MUTEX_INTERFACE; /* On verrouille le mutex */
                    rfm12_receive_handler(bufRX);
                    UNLOCK_MUTEX_INTERFACE;		/* On déverrouille le mutex */
                    xEventGroupClearBits(main_event_group, ER_RFM12);
                }
                E_R_RFM12->result = VIDE;
                recu = VIDE;

                // En cas d'erreur du RFM12, lecture du capteur interne
                if(CMP_ID_RFM12(S_RFM_2) == 0)	{
                    tempo = 0;
#if CONFIG_CAPTEUR_DS18B20
                    errCaptInt = lect_DS18B20(premiere, &temperature);
                    if((errCaptInt == ESP_OK) & (errRFM12 != ESP_OK)) {
                        LOCK_MUTEX_INTERFACE;
                        printf("Erreur du capteur sans fil, on lit le capteur interne.\n");
                        set_temperature(temperature);
                        UNLOCK_MUTEX_INTERFACE;
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
                stopIT_RFM12(E_R_RFM12);
            //ptrEtatObj->obj_modif->fonctionCtrl(ptrEtatObj->obj_modif, premiere);
            //ptrEtatObj->obj_modif = NULL;
            startIT_RFM12(E_R_RFM12);
            //}
            UNLOCK_MUTEX_INTERFACE;		/* On déverrouille le mutex */

            // si perte de signal RFM 12
            // lit les valeurs des capteurs internes et met à jour la "base de données"
            // la fréquence de lecture des capteurs (FREQ_CAPT) doit être supérieure à la fréquence d'émission de RFM12 (4mn)
            if(tempo > FREQ_CAPT) {
                tempo = 0;
                stopIT_RFM12(E_R_RFM12);
#if CONFIG_CAPTEUR_DS18B20
                errCaptInt = lect_DS18B20(premiere, &temperature);
                if(errCaptInt == ESP_OK) {
                    printf("ext\tperdu\tint\t%0.3f", temperature);
                    set_temperature(temperature);
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

/*==========================================================*/
/*											MAIN																*/
/*==========================================================*/
void app_main()
{
    static httpd_handle_t server = NULL;

    vTaskDelay(1000/portTICK_PERIOD_MS);

    const esp_partition_t *partition = esp_ota_get_running_partition();
    /* Mark current app as valid */
    /*
       ESP_LOGW(TAG, "Currently running partition: %s\r\n", partition->label);
       esp_ota_img_states_t ota_state;
       if (esp_ota_get_state_partition(partition, &ota_state) == ESP_OK) {
       if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
       esp_ota_mark_app_valid_cancel_rollback();
       }
       }
     */

    // ---------- INITIALISE la flash NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
        ESP_LOGI(TAG, "Effacement NVS");
    }
    if(ret != ESP_OK) {
        goto erreur;
    }

    // ---------- INITIALISE bus SPI
#ifdef CONFIG_BUS_SPI
    int host_spi;
#ifdef CONFIG_HSPI
    host_spi = SPI2_HOST;
#else
    host_spi = SPI3_HOST;
#endif

    init_SPI(CONFIG_GPIO_MOSI, CONFIG_GPIO_MISO, CONFIG_GPIO_SCLK, CONFIG_GPIO_CS, host_spi);
#endif

    // ---------- INITIALISE bus ONE_WIRE
#ifdef CONFIG_BUS_ONE_WIRE
    gpio_reset_pin(CONFIG_GPIO_ONE_WIRE);
#ifdef CONFIG_ONE_WIRE_INTERNAL_PULLUP
#warning "CONFIG_ONE_WIRE_INTERNAL_PULLUP"
    gpio_set_pull_mode(dht_gpio, GPIO_PULLUP_ONLY);
#else
    gpio_set_pull_mode(CONFIG_GPIO_ONE_WIRE, GPIO_FLOATING);		
#endif
    gpio_set_direction(CONFIG_GPIO_ONE_WIRE, GPIO_MODE_OUTPUT_OD);
    gpio_set_level(CONFIG_GPIO_ONE_WIRE, 1);
#endif

    // ---------- INITIALISE la boucle d'évènements nécessaire pour le wifi, le serveur
    if(esp_event_loop_create_default() != ESP_OK) {
        goto erreur;
    }

    // ---------- INITIALISE le serveur SNTP AVANT INIT WIFI pour utiliser serveur fourni par DHCP
    init_SNTP();

    // ---------- INITIALISE la connection wifi
    if(init_WIFI() != ESP_OK) {
        goto erreur;
    }

    vTaskDelay(1000/portTICK_PERIOD_MS);

    // NOTE delai après reset
    // ++++++++ delai de 10 secondes pour basculer en mode AP pour mise à jour du firmware ++++++
    vTaskDelay(10000 / portTICK_PERIOD_MS);

    // ---------- arret du thread de surveillance de l'état du wifi 
    stop_etat_wifi();

    init_gestionMiseAJour();

    // synchronise l'horloge interne avec serveur SNTP
    maj_Heure();

    ESP_LOGI(TAG, "RAM interne disponible: %" PRIu32 " bytes", esp_get_free_internal_heap_size());
    ESP_LOGW(TAG, "Currently running partition: %s\r\n", partition->label);
    ESP_LOGW(TAG, " ============== FIN INIT : retour main ============== ");
    printf("REBOOT OK : partition %s - mem dispo %lu bytes", partition->label, esp_get_free_internal_heap_size());

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


    // ---------- indication de l'état système via LED_CONN
    EventBits_t uxBits;
    // clignotement rapide CONFIG_GPIO_LED_CON = défaut
    TickType_t delaiTresBref = 75 / portTICK_PERIOD_MS;
    TickType_t delaiBref = 150 / portTICK_PERIOD_MS;
    TickType_t delaiMoyen = 350 / portTICK_PERIOD_MS;
    // flash CONFIG_GPIO_LED2 = fonctionnement correct
    TickType_t delaiLong = 500 / portTICK_PERIOD_MS;

    init_presence_array();

    stats_record_s record;
    struct time t = get_current_time();
    bool preIsPresent = false;
    bool heat = false;

    while(true) {
        //vTaskDelay(delaiLong);

        uxBits = xEventGroupWaitBits(
                main_event_group,       // The event group being tested.
                ER_RFM12 | ER_TEMP_INT, // The bits within the event group to wait for.
                pdFALSE,                // BIT should not be cleared before returning.
                pdFALSE,                // Don't wait for both bits, either bit will do.
                delaiBref );            // Wait a maximum of 100ms for either bit to be set.

        if((uxBits & (ER_RFM12 | ER_TEMP_INT)) | (info_wifi() != ESP_OK)) {
            gpio_set_level(CONFIG_GPIO_LED_CON, 1);
            vTaskDelay(delaiBref);
            gpio_set_level(CONFIG_GPIO_LED_CON, 0);
            vTaskDelay(delaiMoyen);
        }
        else {
            gpio_set_level(CONFIG_GPIO_LED_CON, 1);
            vTaskDelay(delaiLong);
            gpio_set_level(CONFIG_GPIO_LED_CON, 0);
            vTaskDelay(delaiLong);
        }

        double temperaturePresence;
        double temperatureAbsence;
        get_temperature_target(&temperaturePresence, &temperatureAbsence);

        LOCK_MUTEX_INTERFACE /* On verrouille le mutex */
        double temperature;
        get_temperature(&temperature);
        UNLOCK_MUTEX_INTERFACE;		/* On déverrouille le mutex */

        hysteresis_step(temperature, &heat);
        //led_set_level(THERMOSTAT_RELAY_GPIO, heat);
        //led_set_level(THERMOSTAT_LED_GPIO, !heat); // false = on


        if(heat)
        {
            for(int i = 0; i < 5; i++)
            {
                gpio_set_level(CONFIG_GPIO_LED_CON, 1);
                vTaskDelay(delaiTresBref);
                gpio_set_level(CONFIG_GPIO_LED_CON, 0);
                vTaskDelay(delaiTresBref);
            }
        }

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

erreur:
    ESP_LOGE(TAG, " ============== ERREUR INIT main ============== ");
    printf("ERREUR REBOOT");
    while(1) {
        // 		vTaskDelay(10000);
        gpio_set_level(CONFIG_GPIO_LED_CON, 1);
        vTaskDelay(200 / portTICK_PERIOD_MS);
        gpio_set_level(CONFIG_GPIO_LED_CON, 0);
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }

}
