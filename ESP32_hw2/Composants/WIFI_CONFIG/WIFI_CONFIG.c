// ===========================
//  Mode AP
// pour passer en mode  AP : appuyer sur le bouton 'boot' dans les 10 secondes suivant le reset 
// ===========================
// Configuration du wifi :
// 	- mode AP pour entrer SSID et PWD. 
// 			Se connecter au réseau wifi 'esp32'
// 			Accès 192.168.4.1
// 	- si SSID et PWD présent dans NVS :  connection automatique
// ===========================
//  Mise à jour firmware OTA :
// 	- en mode AP uniquement
// 			Se connecter au réseau wifi 'esp32'
// 			Accès 192.168.4.1/update
// 			uploader le fichier bin de mise à jour


#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "cJSON.h"
#include "driver/gpio.h"
#include <esp_http_server.h>
#include <sys/param.h>
#include "lwip/err.h"
#include "lwip/sys.h"
#include <esp_ota_ops.h>

#include "WIFI_CONFIG.h"

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* defines */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*  constantes */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
static const char *TAG = "CONF_Wifi";

// page HTML pour configuration du wifi
char pageConfigDeb[] = {"\
<!DOCTYPE html><html>\
<meta charset=\"utf-8\" name=\"viewport\" content=\"width=device-width, initial-scale=1.0, shrink-to-fit=no\"></meta>\
<head>\
<style>\
form {display: grid;padding: 1em; background: #f9f9f9; border: 1px solid #c1c1c1; margin: 2rem auto 0 auto; max-width: 400px; padding: 1em;}\
form input {background: #fff;border: 1px solid #9c9c9c;}\
form button {background: lightgrey; padding: 0.7em; width: 120px; border: 0;}\
label, a {padding: 0.5em 0.5em 0.5em 0;}\
input {padding: 0.7em;margin-bottom: 0.5rem;}\
input:focus {outline: 10px solid gold;}\
@media (min-width: 300px) {form {grid-template-columns: 150px 1fr; grid-gap: 16px;}label {text-align: right; grid-column: 1 / 2;}input, button, p {grid-column: 2 / 3;}}\
</style>\
</head>\
<body>\
<form class=\"form1\" id=\"loginForm\" action=\"/connection\">\
<label for=\"SSID\">Réseau Wifi</label>\
<input id=\"ssid\" type=\"text\" name=\"ssid\" maxlength=\"64\" minlength=\"4\" value=\
"};

char pageConfigFin[] = {"\
>\
<label for=\"Password\">Mot de passe</label>\
<input id=\"pwd\" type=\"text\" name=\"pwd\" maxlength=\"64\" minlength=\"4\">\
<button id=\"btnEnreg\">Enregitrer</button>\
<label > Accès au serveur : </label><a href= http://esp32/> http://esp32/ </a>\
<p id=\"info\">_</p>\
</form>\
<form class=\"form\" action=\"annuler\" method=\"get\">\
<button type=\"button\" onclick=\"annuler()\" id=\"btnANNULER\" >Annuler</button>\
</form>\
<script>\
	var serveur = 'http://' + location.host + \"/\";\
	document.getElementById(\"loginForm\").addEventListener(\"submit\", (e) => {\
			 e.preventDefault();\
			 const formData = new FormData(e.target);\
			 const data = Array.from(formData.entries()).reduce((memo, pair) => ({...memo, [pair[0]]: pair[1],}), {});\
		 var xhr = new XMLHttpRequest();\
		 xhr.open(\"POST\", serveur + \"connection\", true);\
		 xhr.setRequestHeader('Content-Type', 'application/json');\
		xhr.onload = function() {\
			let responseObj = xhr.response;\
			document.getElementById(\"info\").innerHTML = \"ENREGISTRE dans NVS, tentative de connection en cours\";\
			document.getElementById(\"btnEnreg\").disabled = true;\
			document.getElementById(\"btnANNULER\").disabled = true;\
		};\
		 xhr.send(JSON.stringify(data));\
	 }\
 );\
	function annuler() {\
		var xhr = new XMLHttpRequest();\
		xhr.open(\"POST\", serveur + \"annuler\", true);\
		xhr.onload = function() {\
			document.getElementById(\"info\").innerHTML = \"ANNULATION tentative de connection en cours\";\
			document.getElementById(\"btnEnreg\").disabled = true;\
			document.getElementById(\"btnANNULER\").disabled = true;\
		};\
		xhr.send();\
	}\
</script>\
</body></html>\
"};

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*  variables externes */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*  variables locales */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
EventGroupHandle_t STA_wifi_event_group;	// groupe d'évènement de la STAtion

static char __SSID[32];
static char	__PWD[64];
static char adresse_AP[20];				// adresse IP du host serveur
static httpd_handle_t serveurHTTP_wifi = NULL;

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*  fonctions locales */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
void wifi_init_STA(void);
void wifi_init_softap(void);

/* ================= serveur HTTP ====================== */
/*==========================================================*/
/* Envoie la page html pour choix du programme à télécharger */ /*  */
/*==========================================================*/
extern const uint8_t index_html_start[] asm("_binary_update_html_start");
extern const uint8_t index_html_end[] asm("_binary_update_html_end");
esp_err_t pageUpdate(httpd_req_t *req)
{
	httpd_resp_send(req, (const char *) index_html_start, index_html_end - index_html_start);
	httpd_resp_send_chunk(req, NULL, 0);
	return ESP_OK;
}

/*==========================================================*/
/* telecharge le fichier .bin pour mise à jour du firmware */ /*  */
/*==========================================================*/
esp_err_t updatePost(httpd_req_t *req)
{
	char buf[1000];
	esp_ota_handle_t ota_handle;
	int remaining = req->content_len;

	const esp_partition_t *ota_partition = esp_ota_get_next_update_partition(NULL);
	ESP_ERROR_CHECK(esp_ota_begin(ota_partition, OTA_SIZE_UNKNOWN, &ota_handle));

	while (remaining > 0) {
		int recv_len = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)));

		// Timeout Error: Just retry
		if (recv_len == HTTPD_SOCK_ERR_TIMEOUT) {
			continue;

		// Serious Error: Abort OTA
		} else if (recv_len <= 0) {
			httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Protocol Error");
			return ESP_FAIL;
		}

		// Successful Upload: Flash firmware chunk
		if (esp_ota_write(ota_handle, (const void *)buf, recv_len) != ESP_OK) {
			httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Flash Error");
			return ESP_FAIL;
		}

		remaining -= recv_len;
	}

	// Validate and switch to new OTA image and reboot
	if (esp_ota_end(ota_handle) != ESP_OK || esp_ota_set_boot_partition(ota_partition) != ESP_OK) {
			httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Validation / Activation Error");
			return ESP_FAIL;
	}

	httpd_resp_sendstr(req, "Firmware update complete, rebooting now!\n");
	httpd_resp_send_chunk(req, NULL, 0);

	vTaskDelay(500 / portTICK_PERIOD_MS);
	esp_restart();

	return ESP_OK;
}

/*==========================================================*/
/* Envoie la page de saisie SSID et PWD pour se connecter au wifi */ /*  */
/*==========================================================*/
static esp_err_t page_choix_reseau(httpd_req_t *req)
{
	httpd_resp_sendstr_chunk(req, pageConfigDeb);
	httpd_resp_sendstr_chunk(req, __SSID);
	httpd_resp_sendstr_chunk(req, pageConfigFin);
	httpd_resp_send_chunk(req, NULL, 0);
	return ESP_OK;
}

/*==========================================================*/
/* Enregistre SSID et PWD saisies */ /*  */
/*==========================================================*/
static esp_err_t page_enreg_SSID_PSW(httpd_req_t *req)
{
    char buf[128];
    int ret, remaining = req->content_len;

    while (remaining > 0)
    {
        /* lecture des data de la requête */
        if ((ret = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)))) <= 0)
        {
            if (ret == 0)
            {
                ESP_LOGI(TAG, "Pas de contenu, réessayer ...");
            }
            else if (ret == HTTPD_SOCK_ERR_TIMEOUT)
            {
                /* recommencer si timeout */
                continue;
            }
            return ESP_FAIL;
        }

        /* Log data received */
        ESP_LOGI(TAG, "=========== Données reçues nouvelle config ==========");
//         ESP_LOGI(TAG, "%.*s", ret, buf);
//         ESP_LOGI(TAG, "=====================================================");
        
        cJSON *root = cJSON_Parse(buf);

        sprintf(__SSID, "%s", cJSON_GetObjectItem(root, "ssid")->valuestring);
        sprintf(__PWD, "%s", cJSON_GetObjectItem(root, "pwd")->valuestring);

        ESP_LOGI(TAG, "ssid: %s pwd: %s", __SSID, __PWD);
        ESP_LOGI(TAG, "=====================================================");

        remaining -= ret;
    }

    // fin -> response
    httpd_resp_send_chunk(req, NULL, 0);
		
		wifi_init_STA();
		
    return ESP_OK;
}

/*==========================================================*/
/* Annulation de la saisie/modification de la connection wifi */ /*  */
/*==========================================================*/
static esp_err_t page_annulation(httpd_req_t *req) {
// 	ESP_LOGI(TAG, "Page d'annulation");
	httpd_resp_send(req, "ANNULATION modification des parametres WIFI.", HTTPD_RESP_USE_STRLEN);
	vTaskDelay(2000 / portTICK_PERIOD_MS);
	ESP_ERROR_CHECK(esp_wifi_stop() );
	wifi_init_STA();
	return ESP_OK;
}

/*==========================================================*/
/* Enregistre le gestionnaire d'URI pour la page de choix du réseau wifi */ /*  */
/*==========================================================*/
static const httpd_uri_t URI_choix_reseau = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = page_choix_reseau,
    .user_ctx = NULL};
/*==========================================================*/
/* Enregistre le gestionnaire d'URI pour l'enregistrement de SSID et PWD */ /*  */
/*==========================================================*/
static const httpd_uri_t URI_enreg_SSID_PSW = {
    .uri = "/connection",
    .method = HTTP_POST,
    .handler = page_enreg_SSID_PSW,
    .user_ctx = "TEST"};
/*==========================================================*/
/* Enregistre le gestionnaire d'URI pour l'annulation */ /*  */
/*==========================================================*/
static const httpd_uri_t URI_annuler = {
    .uri = "/annuler",
    .method = HTTP_POST,
    .handler = page_annulation,
    .user_ctx = NULL};

/*==========================================================*/
/* Enregistre le gestionnaire d'URI de la page mise à jour OTA */ /*  */
/*==========================================================*/
httpd_uri_t _pageUpdate = {
	.uri	  = "/update",
	.method   = HTTP_GET,
	.handler  = pageUpdate,
	.user_ctx = NULL
};

/*==========================================================*/
/* Enregistre le gestionnaire d'URI de la mise à jour OTA */ /*  */
/*==========================================================*/
httpd_uri_t _updatePost = {
	.uri	  = "/updatePost",
	.method   = HTTP_POST,
	.handler  = updatePost,
	.user_ctx = NULL
};

/*==========================================================*/
/* Démarre le serveur http */ /*  */
/*==========================================================*/
httpd_handle_t start_serveur_config_wifi(void)
{
    // initialise la configuration par défaut du serveur HTTP
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    
	if (httpd_start(&serveurHTTP_wifi, &config) == ESP_OK) {
		ESP_LOGI(TAG, "Démarrage du serveur conf wifi sur le port: '%d'", config.server_port);
        // enregistre les gestionnaires d'URI
		httpd_register_uri_handler(serveurHTTP_wifi, &URI_choix_reseau);
		httpd_register_uri_handler(serveurHTTP_wifi, &URI_enreg_SSID_PSW);
		httpd_register_uri_handler(serveurHTTP_wifi, &URI_annuler);
		httpd_register_uri_handler(serveurHTTP_wifi, &_pageUpdate);
		httpd_register_uri_handler(serveurHTTP_wifi, &_updatePost);
		return serveurHTTP_wifi;
	}

	ESP_LOGW(TAG, "Erreur de démarrage du serveur conf wifi !");
	return NULL;
}

/*==========================================================*/
/* Arrete le serveur http */ /*  */
/*==========================================================*/
void stop_serveur_config_wifi(void)
{
	if (serveurHTTP_wifi != NULL) {
		ESP_ERROR_CHECK(httpd_stop(serveurHTTP_wifi));
		serveurHTTP_wifi = NULL;
		ESP_LOGI(TAG, "Arrêt du serveur conf wifi");
	}
	else {
// 		ESP_LOGW(TAG, "serveur conf wifi serveurHTTP_wifi est NULL");
	}
}

/* ================= connection wifi ====================== */
/*==========================================================*/
/* Reset la connection wifi */ /* 
		utilisé lors du changement de mode STA/AP */
/*==========================================================*/
static void reset_wifi(void){
// 	ESP_LOGW(TAG, "reset_wifi");
	ESP_ERROR_CHECK(esp_wifi_stop() );
	ESP_ERROR_CHECK(esp_wifi_start() );
}

/*==========================================================*/
/* Gestion des évènements wifi */ /* 
	Surveille l'état de la connection wifi et se connecte au réseau wifi enregistré */
/*==========================================================*/
static void STA_wifi_event(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
	static int	nbr_essai_conn = 0;		// compteur du nombre de tentatives de connection
	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
// 		ESP_LOGI(TAG, "WIFI STA a demarre : 1er essai de connection à %s", __SSID);
		esp_wifi_connect();
	} else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
		if (nbr_essai_conn < CONFIG_NOMBRE_TENTATIVE_CONNECTION) {
			esp_wifi_connect();
			nbr_essai_conn++;
// 			ESP_LOGI(TAG, "Essai num %i de connection à %s", nbr_essai_conn+1, __SSID);
		} else {
			xEventGroupSetBits(STA_wifi_event_group, WIFI_FAIL_BIT);
			xEventGroupClearBits(STA_wifi_event_group, WIFI_CONNECTED_BIT);
// 			ESP_LOGI(TAG,"Erreur de connection au réseau wifi");
		}
	} else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
		ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
		xEventGroupSetBits(STA_wifi_event_group, WIFI_CONNECTED_BIT);
		ESP_LOGW(TAG, "connecté avec ip:" IPSTR, IP2STR(&event->ip_info.ip));
		stop_serveur_config_wifi();
		nbr_essai_conn = 0;
	}
}
/*==========================================================*/
/* Initialise le wifi en mode STA et se connecte au réseau wifi enregistré */ /* 
	Si SSID est vide bascule en mode AP pour entrer SSID et PWD */
/*==========================================================*/
void wifi_init_STA()
{
	if (strlen(__SSID) != 0) {
		ESP_LOGW(TAG, "wifi_init_STA ESP_WIFI_MODE_STA réseau: %s" , __SSID);
		ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

		wifi_config_t wifi_config = {
				.sta = {
						/* Authmode threshold resets to WPA2 as default if password matches WPA2 standards (pasword len => 8).
						 * If you want to connect the device to deprecated WEP/WPA networks, Please set the threshold value
						 * to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK and set the password with length and format matching to
			 			 * WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK standards.
// 								.threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
						 */
						.threshold.authmode = WIFI_AUTH_OPEN,
						.sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
				},
		};
		strcpy((char *)wifi_config.sta.ssid, __SSID);
		strcpy((char *)wifi_config.sta.password, __PWD);

		ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
		reset_wifi();

		/* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
		 * number of re-tries (WIFI_FAIL_BIT). The bits are set by STA_wifi_event() (see above) */
		EventBits_t bits = xEventGroupWaitBits(STA_wifi_event_group,
																						WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
																						pdFALSE,
																						pdFALSE,
																						portMAX_DELAY);

		/* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually happened. */
		if (bits & WIFI_CONNECTED_BIT) {
// 			ESP_LOGI(TAG, "connecté au réseau SSID:%s", __SSID);
		} else if (bits & WIFI_FAIL_BIT) {
			ESP_LOGI(TAG, "Erreur connection à SSID:%s", (char *)wifi_config.sta.ssid);
		} else {
				ESP_LOGE(TAG, "Evènement inattendu");
		}
  }
  else {
  	ESP_LOGI(TAG, "      ==============================================================");
  	ESP_LOGI(TAG, "Pour entrer SSID et password, se connecteur au réseau wifi %s, URL : %s", CONFIG_SSID_POINT_ACCES, adresse_AP);
  	ESP_LOGI(TAG, "      ==============================================================");
		wifi_init_softap();
  }
  return;
}

/*==========================================================*/
/* Initialise le wifi en mode AP pour entrer SSID et PWD */ /*  */
/*==========================================================*/
void wifi_init_softap()
{
// 	ESP_LOGW(TAG, "wifi_init_softap ESP_WIFI_MODE_AP");
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
	xEventGroupClearBits(STA_wifi_event_group, WIFI_CONNECTED_BIT);

	wifi_config_t wifi_config = {
			.ap = {
					.ssid = CONFIG_SSID_POINT_ACCES,
					.ssid_len = strlen(CONFIG_SSID_POINT_ACCES),
					.channel = CONFIG_CANAL_WIFI_AP,
					.password = CONFIG_MOT_PASSE_POINT_ACCES,
					.max_connection = CONFIG_NBR_MAX_CONNECTIONS_AP,
					.authmode = WIFI_AUTH_WPA_WPA2_PSK,
					.pmf_cfg = {
					.required = false,
					},
			},
	};
	if (strlen(CONFIG_MOT_PASSE_POINT_ACCES) == 0) {
		wifi_config.ap.authmode = WIFI_AUTH_OPEN;
	}

	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
	reset_wifi();
	ESP_LOGI(TAG, "Point d'Accès démarré  SSID: %s password %s channel: %d",
					 CONFIG_SSID_POINT_ACCES, CONFIG_MOT_PASSE_POINT_ACCES, CONFIG_CANAL_WIFI_AP);
		start_serveur_config_wifi();

}

/*==========================================================*/
/* Retourne l'état de la connection wifi: ESP_OK ou ESP_FAIL */ /*  */
/*==========================================================*/
int info_wifi(void){
	EventBits_t uxBits;
	const TickType_t xTicksToWait = 5000 / portTICK_PERIOD_MS;
	uxBits = xEventGroupWaitBits(STA_wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, xTicksToWait);
	if(uxBits & WIFI_CONNECTED_BIT) {
// 		ESP_LOGI(TAG, "info_wifi: WIFI connecté");
		return ESP_OK;
	}
	ESP_LOGI(TAG, "info_wifi: WIFI déconnecté");
	return ESP_FAIL;
}

#if(CONFIG_TACHE_ETAT_WIFI)
/* ================= TACHE ETAT WIFI ====================== */
/* tache de fond pour visualiser l'état de la connection et changer la configuration */ /* 
		- utilise une led (définie dans menuconfig) pour indiquer l'état du wifi:
				- clignotement rapide : pas de connection
				- clignotement lent: wifi connecté
		- utilise un bouton (définie dans menuconfig) pour basculer en mode AP et configurer le wifi */
/*==========================================================*/
static void etat_wifi(void * pvParameters)
{
// 	ESP_LOGW(TAG, "tache etat wifi demarré");
	xEventGroupSetBits(STA_wifi_event_group, WIFI_RUN_ETAT_BIT);
	EventBits_t uxBits;
	const TickType_t delaiBref = 300 / portTICK_PERIOD_MS;
	const TickType_t delaiLong = 2000 / portTICK_PERIOD_MS;
	while(xEventGroupGetBits(STA_wifi_event_group) & WIFI_RUN_ETAT_BIT){
		#if(!CONFIG_LED_CONNECTION & !CONFIG_BOUTON_AP)
			vTaskDelay(delaiLong);
		#endif
		#if(CONFIG_LED_CONNECTION)
			gpio_set_level(CONFIG_GPIO_LED_CON, 1);
			vTaskDelay(delaiBref);
			gpio_set_level(CONFIG_GPIO_LED_CON, 0);

			uxBits = xEventGroupGetBits(STA_wifi_event_group);
			if((uxBits & WIFI_CONNECTED_BIT) != 0 ) 
			{
				vTaskDelay(delaiLong);
			}
			else
			{
				vTaskDelay(delaiBref);
			}
		#endif
		#if(CONFIG_BOUTON_AP)
			if(gpio_get_level(CONFIG_GPIO_BTN_AP) == 0) {
				ESP_LOGW(TAG, "bouton pressé");
				ESP_ERROR_CHECK(esp_wifi_stop());
				wifi_init_softap();
			}
		#endif
	}
	vTaskDelete(NULL); // IMPERATIF pour arrêter la tâche
}
#endif

/*==========================================================*/
/* démarre la tâche "etat_wifi" pour indiquer l'état de la connection wifi ou modifier la configuration  */ /*  */
/*==========================================================*/
void run_etat_wifi(void)
{
	TaskHandle_t wifiHandle = NULL;		// tache pour indiquer l'état de la connection wifi ou modifier la configuration
	xTaskCreate(etat_wifi, "Etat_Wifi", 4096, NULL, 1, &wifiHandle );
  return;
}

/*==========================================================*/
/* arrête  la tâche "etat_wifi" */ /*  */
/*==========================================================*/
void stop_etat_wifi(void)
{
// 	ESP_LOGI(TAG, "demande arret etat_wifi");
	if(xEventGroupGetBits(STA_wifi_event_group) & WIFI_RUN_ETAT_BIT) {
		xEventGroupClearBits(STA_wifi_event_group, WIFI_RUN_ETAT_BIT);
	}
  return;
}

/* ================= INITIALISATION WIFI ====================== */
/*==========================================================*/
/* Initialise la connection au wifi */ /* 
		nécessite que esp_event_loop_create_default() soit initialisé au préalable */
/*==========================================================*/
int init_WIFI(void) {
	__SSID[0] = 0;
	__PWD[0] = 0;
	
	// initialisation du groupe d'événements nécessaire avant création de la tache 
	// d'indication d'état de la connection wifi et du bouton de modification
	STA_wifi_event_group = xEventGroupCreate();
	xEventGroupClearBits(STA_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT | WIFI_RUN_ETAT_BIT | WIFI_STOP_ETAT_BIT);
	
#if(CONFIG_TACHE_ETAT_WIFI)
	#if(CONFIG_LED_CONNECTION)
	gpio_reset_pin(CONFIG_GPIO_LED_CON);	/* Set the GPIO as a push/pull output */
	gpio_set_direction(CONFIG_GPIO_LED_CON, GPIO_MODE_OUTPUT);
	gpio_set_level(CONFIG_GPIO_LED_CON, 1);
	#endif
	#if(CONFIG_BOUTON_AP)
	gpio_reset_pin(CONFIG_GPIO_BTN_AP);
	gpio_set_direction(CONFIG_GPIO_BTN_AP, GPIO_MODE_INPUT);
	#endif
  run_etat_wifi();
#endif

	ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,    &STA_wifi_event, NULL));
	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT,   IP_EVENT_STA_GOT_IP, &STA_wifi_event, NULL));

	ESP_ERROR_CHECK(esp_netif_init());
	esp_netif_t * p_netif_ap = esp_netif_create_default_wifi_ap();
	esp_netif_t * p_netif_sta = esp_netif_create_default_wifi_sta();
	esp_netif_set_hostname(p_netif_ap, CONFIG_NOM_HOTE_MDNS);
	esp_netif_set_hostname(p_netif_sta, CONFIG_NOM_HOTE_MDNS);
	
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
 
	/* exécuter "esp_wifi_get_config" après "esp_wifi_init" : si PWD et SSID existent dans NVS, les récupérer */
	wifi_config_t conf_NVS;
	esp_wifi_get_config(ESP_IF_WIFI_STA, &conf_NVS);
	if(strlen((char*)&conf_NVS.sta.ssid) != 0) {
		strcpy(__SSID, (char *)conf_NVS.sta.ssid);
		strcpy(__PWD, (char *)conf_NVS.sta.password);
// 		ESP_LOGI(TAG, "Configuration Wifi enregistrée dans la partition flash NVS");
// 		ESP_LOGI(TAG, "flash NVS SSID: %s PWD: %s", __SSID, __PWD);
		wifi_init_STA();
	}
	else {
		ESP_LOGI(TAG, "configuration non trouvée dans NVS ou serveur, long SSID : %i", strlen((char*)&conf_NVS.sta.ssid));    
		wifi_init_softap();
	/* recherche l'adresse IP du point d'accès */
		esp_netif_ip_info_t if_info;
		ESP_ERROR_CHECK(esp_netif_get_ip_info(p_netif_ap, &if_info));
		esp_ip4addr_ntoa(&if_info.ip, (char*)adresse_AP, sizeof(adresse_AP));
		ESP_LOGI(TAG, "Access Point ESP32 IP: %s \n", adresse_AP);
//     ESP_LOGI(TAG, "Passerelle: " IPSTR , IP2STR(&if_info.gw));
//     ESP_LOGI(TAG, "Masque de reseau: " IPSTR "\n", IP2STR(&if_info.netmask));

		// NOTE ajout pour permettre la première connection au Wifi
		strcpy(__SSID, "reseau");
		strcpy(__PWD, "mot de passe");

// 		while(info_wifi() != ESP_OK) {
// 			vTaskDelay(500 / portTICK_PERIOD_MS);
// 		}
// 
	}

	// NOTE modif 
	// attente que la connection WIFI soit établie
	for(int retry = 0; retry < 10; retry++)
    {
        if (info_wifi() == ESP_OK)
            break;
		vTaskDelay(500 / portTICK_PERIOD_MS);
	}

	return info_wifi();
}

