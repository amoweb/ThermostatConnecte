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

#if(CONFIG_LED_RGB)
#include "led_rgb.h"
#endif


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

	// ============ Boucle principale ===============
	/* durée de la boucle ESP 32 : FREQ_BCLE ms
			surveille les entrées html ou RFM12
			lit les capteurs internes
			efectue la mise à jour de l'interface html et du hardware
	*/
		
	while(1) {	
		//ptrEtatObj = premiere;
// 		tempo++;
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
					//hardRFM12(premiere, bufRX);
						UNLOCK_MUTEX_INTERFACE;		/* On déverrouille le mutex */
					xEventGroupClearBits(main_event_group, ER_RFM12);
				}
				E_R_RFM12->result = VIDE;
				recu = VIDE;
			
				// lecture capteurs internes après réception RFM12 DS18B20 et mise à jour "base de données"
				if(CMP_ID_RFM12(S_RFM_2) == 0)	{
					tempo = 0;
				#if CONFIG_TYPE_DHT
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
                // DEBUG : garder la partie ci-dessous
				#if CONFIG_CAPTEUR_DS18B20
					// NOTE :  le délai de conversion du DS18B20 est de 750 ms
					errCaptInt = lect_DS18B20(premiere, &temperature);
					if((errCaptInt == ESP_OK) & (errRFM12 == ESP_OK)) {
                        printf("ext\t%0.3f\tint\t%0.3f\n", tempRFM12, temperature);
						printf("ext\t%0.3f\tint\t%0.3f", tempRFM12, temperature);
					}
					else if (errCaptInt == ESP_OK) {
						printf("ext\terreur\tint\t%0.3f", temperature);
                        printf("ext\terreur\tint\t%0.3f\n", temperature);
					}
					else {
				  	printf("ext\t%0.3f\tint\terreur", tempRFM12);
                        printf("ext\t%0.3f\tint\terreur\n", tempRFM12);
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
                        printf("maj\n");
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
		#if CONFIG_TYPE_DHT
				errCaptInt = lect_dth(premiere, &temperature, &humidity);
				if(errCaptInt == ESP_OK) {
					printf("ext\tperdu\tint\t%0.3f\t%0.3f", temperature, humidity);
				}
		#endif
		#if CONFIG_CAPTEUR_DS18B20
				errCaptInt = lect_DS18B20(premiere, &temperature);
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

/*==========================================================*/
/* Initialise SPIFFS */ /* 
	return ESP_OK si SPIFFS est initialisé */
/*==========================================================*/
esp_err_t init_spiffs(void)
{
//   ESP_LOGI(TAG, "Initialisation SPIFFS");

  esp_vfs_spiffs_conf_t conf = {
  .base_path = "",
  .partition_label = NULL,
  .max_files = 5,   // This sets the maximum number of files that can be open at the same time
  .format_if_mount_failed = true
  };

  esp_err_t ret = esp_vfs_spiffs_register(&conf);
  if (ret != ESP_OK) {
		if (ret == ESP_FAIL) {
			ESP_LOGE(TAG, "Erreur montage system de fichiers");
		} else if (ret == ESP_ERR_NOT_FOUND) {
			ESP_LOGE(TAG, "Erreur recherche partition SPIFFS");
		} else {
			ESP_LOGE(TAG, "Erreur Initialisation SPIFFS (%s)", esp_err_to_name(ret));
		}
		return ret;
  }

  size_t total = 0, used = 0;
  ret = esp_spiffs_info(NULL, &total, &used);
  if (ret != ESP_OK) {
		ESP_LOGE(TAG, "Erreur d'accès aux informations de la partition SPIFFS (%s)", esp_err_to_name(ret));
		return ret;
  }

//   ESP_LOGW(TAG, "Taille partition : total = %d - utilise = %d", total, used);
  return ESP_OK;
}

/*==========================================================*/
/*											MAIN																*/
/*==========================================================*/
void app_main()
{
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
   
		/* ---------- INITIALISE file storage */
	if(init_spiffs() != ESP_OK) {
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

	// variable pour surveillance état système
	// !!! à initialiser avant création thread mise à jour
	main_event_group = xEventGroupCreate();
	xEventGroupClearBits(main_event_group, MAIN_OK | ER_RFM12 | ER_TEMP_INT);
	int bcltempo = 0;
	// ---------- INITIALISE thread de mise à jour
	// NOTE utiliser tskIDLE_PRIORITY pour éviter l'appel watchdog, mais utilise tout le temps processeur !
	xTaskCreatePinnedToCore(gestionMiseAJour, "threadMAJ", 10000, NULL, 1, NULL, 1);

	
	// synchronise l'horloge interne avec serveur SNTP
	maj_Heure();
	
	ESP_LOGI(TAG, "RAM interne disponible: %" PRIu32 " bytes", esp_get_free_internal_heap_size());
	ESP_LOGW(TAG, "Currently running partition: %s\r\n", partition->label);
	ESP_LOGW(TAG, " ============== FIN INIT : retour main ============== ");
	printf("REBOOT OK : partition %s - mem dispo %lu bytes", partition->label, esp_get_free_internal_heap_size());

	/*  // necessite configUSE_TRACE_FACILITY and configUSE_STATS_FORMATTING_FUNCTIONS 
// 	char stat[1024];
// 		vTaskList(stat);
// 		printf("%s", stat);
// 		vTaskDelay(10000 / portTICK_PERIOD_MS);
	*/
		
	// ---------- indication de l'état système via LED_CONN
	EventBits_t uxBits;
	// clignotement rapide CONFIG_GPIO_LED_CON = défaut
	TickType_t delaiBref = 150 / portTICK_PERIOD_MS;
	TickType_t delaiMoyen = 350 / portTICK_PERIOD_MS;
	// flash CONFIG_GPIO_LED2 = fonctionnement correct
	TickType_t delaiLong = 500 / portTICK_PERIOD_MS;

	while(1) {
  	uxBits = xEventGroupWaitBits(
                 main_event_group,    								// The event group being tested.
                 ER_RFM12 | ER_TEMP_INT,  	// The bits within the event group to wait for.
                 pdFALSE,         										// BIT should not be cleared before returning.
                 pdFALSE,        											// Don't wait for both bits, either bit will do.
                 delaiBref ); 												// Wait a maximum of 100ms for either bit to be set.

		if((uxBits & (ER_RFM12 | ER_TEMP_INT)) | (info_wifi() != ESP_OK)) {
			gpio_set_level(CONFIG_GPIO_LED_CON, 1);
			vTaskDelay(delaiBref);
			gpio_set_level(CONFIG_GPIO_LED_CON, 0);
			vTaskDelay(delaiMoyen);
		}
		else {
			if(++bcltempo > 6) {
				gpio_set_level(CONFIG_GPIO_LED2, 1);
				vTaskDelay(delaiBref);
				gpio_set_level(CONFIG_GPIO_LED2, 0);
				vTaskDelay(delaiMoyen);
				bcltempo = 0;
			}
			vTaskDelay(delaiLong);
		}
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
