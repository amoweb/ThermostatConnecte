#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include <esp_http_server.h>
#include <esp_wifi.h>
#include <pthread.h>
#include <sys/param.h>
#include <time.h>

#include	"serveur.h"
#include	"WIFI_CONFIG.h"
#include  "HEURE_SNTP.h"
#include	"serveurFichier.h"
#include	"interface.h"

////////////////////////////////////////////////////////////////////////////
//	define - typedef
////////////////////////////////////////////////////////////////////////////
typedef struct _donnee_serveur {
  /* Tampon pour le stockage temporaire pendant le transfert de fichiers */
  char tampon[TAILLE_TAMPON];
//   char bufEmis[TAILLE_TAMPON/2];
  _etatObj *structMAJ;
}_donnee_serveur;

////////////////////////////////////////////////////////////////////////////
//  constantes
////////////////////////////////////////////////////////////////////////////
static const char *TAG = "ESP32_serveur";

////////////////////////////////////////////////////////////////////////////
//  fonctions localess
////////////////////////////////////////////////////////////////////////////
static httpd_handle_t start_webserver(void);
esp_err_t stop_webserver(httpd_handle_t server);

////////////////////////////////////////////////////////////////////////////
//  variables locales
////////////////////////////////////////////////////////////////////////////
pthread_mutex_t mutexInterface;
// !!! TAILLE_LOG 
#define	TAILLE_LOG		8192	// 2048
#define TAILLE_LIGNE	60		// nbr de caractère par ligne hors '\n' et '\Ø'
char logInfo[TAILLE_LOG] = "";

/*==========================================================*/
/*  */ /*  */
/*==========================================================*/
/*==========================================================*/
/*	concatInfo */  /* 
	concatène les messages LOGD, LOGA
 */
/*==========================================================*/
void concatInfo(const char *format, ...) {
	char buf[82];
	va_list args;
	char strftime_buf[20];
	time_t now;
	struct tm timeinfo;

			LOCK_MUTEX_INTERFACE; 	/* >>>>>>>>>>>> On verrouille le mutex */
	va_start(args, format);
  vsprintf(buf, format, args);
	va_end(args);
	int len = strlen(buf);
	if(len >= TAILLE_LIGNE) {
		buf[TAILLE_LIGNE+1] = 0x0A;
		buf[TAILLE_LIGNE+2] = '\0';
		len = TAILLE_LIGNE + 1;
	}
	if(strlen(logInfo) > (TAILLE_LOG -TAILLE_LIGNE -20 -2)) {
		logInfo[0] = '\0';
	}

	time(&now);
	localtime_r(&now, &timeinfo);
 	snprintf(strftime_buf, 20, "%.2d:%.2d:%.2d\t", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

	strncat(logInfo, strftime_buf, 20);
	strncat(logInfo, buf, len);
			UNLOCK_MUTEX_INTERFACE	 /* <<<<<<<<<< On déverrouille le mutex */	
}

/*==========================================================*/
/*	retourne les infos de DBG  */  /*  */
/*==========================================================*/
esp_err_t infoDBG(httpd_req_t *req)
{
			LOCK_MUTEX_INTERFACE; 	/* >>>>>>>>>>>> On verrouille le mutex */

  // NOTE ferme la connection pour éviter httpd_sock_err: error in send : 104 ?
  httpd_resp_set_hdr(req, "Connection", "close");
    
	httpd_resp_sendstr_chunk(req, logInfo);
	logInfo[0] = '\0';
			UNLOCK_MUTEX_INTERFACE	 /* <<<<<<<<<< On déverrouille le mutex */
	httpd_resp_send_chunk(req, NULL, 0);
	return ESP_OK;
}

//==========================================================
/* Crée un fichier json et le retourne à la page web index */ /* 
		fait appel à creerJsonPageHTML de interfaca
		Attention à la taille du fichier json */
/*==========================================================*/
esp_err_t initPageHTML(httpd_req_t *req)
{
	char *json = (( _donnee_serveur *)req->user_ctx)->tampon;
	creerJsonPageHTML(json);

  // NOTE ferme la connection pour éviter httpd_sock_err: error in send : 104 ?
  httpd_resp_set_hdr(req, "Connection", "close");
	httpd_resp_set_type(req, "application/json;charset=UTF-8");
	httpd_resp_sendstr_chunk(req, json);
	httpd_resp_send_chunk(req, NULL, 0);
	return ESP_OK;
}

//==========================================================
/* Retourne l'état matériel à la page web index */ /* 
	Fait appel à creerJsonEtatHard de interface
	Retour un fichier json (attention à sa taille) */
/*==========================================================*/
esp_err_t renvoiEtat(httpd_req_t *req)
{
	char *jsonMAJ = (( _donnee_serveur *)req->user_ctx)->tampon;

	creerJsonEtatHard(jsonMAJ, (_etatObj *) (( _donnee_serveur *)req->user_ctx)->structMAJ );

  // NOTE ferme la connection pour éviter httpd_sock_err: error in send : 104 ?
	httpd_resp_set_hdr(req, "Connection", "close");
	httpd_resp_set_type(req, "application/json;charset=UTF-8");
	httpd_resp_sendstr_chunk(req, jsonMAJ);
	httpd_resp_send_chunk(req, NULL, 0);
	return ESP_OK;
}

//==========================================================
/* Effectue la mise à jour des structures chainées _etatObj  */ /* 
		suite à une action dans la page web index  
		Retourne l'état matériel à la page web au format json */
/*==========================================================*/
esp_err_t miseAJour(httpd_req_t *req)
{
	char *content = (( _donnee_serveur *)req->user_ctx)->tampon;
	content[0] = '\0';
	char *ptrcontent = content;
	int arecevoir = req->content_len;
	int recept = 0;
	// tronque si la longueur du contenu est supérieure au buffer
	size_t recv_size = MIN(req->content_len, TAILLE_TAMPON);

	while(arecevoir > 0) {
		if ((recept = httpd_req_recv(req, ptrcontent, recv_size)) <= 0){
			if (recept == HTTPD_SOCK_ERR_TIMEOUT) {
				continue;
			}
			ESP_LOGI(TAG, "ERREUR reception misAJour");
			httpd_resp_set_hdr(req, "Connection", "close");
			httpd_resp_send_chunk(req, NULL, 0);
			return ESP_FAIL;
		}
		arecevoir -= recept;
		ptrcontent += recept;
	}
	// NOTE ajouter '\0' pour pouvoir utiliser le contenu comme un string.
	content[recept] = '\0';

	// mise à jour de l'état matériel
	miseAJourWeb(content, (( _donnee_serveur *)req->user_ctx)->structMAJ);
// 	ESP_LOGI(TAG, "miseAJourWeb : OK content %s", content);
	renvoiEtat(req);
	return ESP_OK;
}

/*==========================================================*/
/* gestion des erreurs 404 */ /* 
	retourne ESP_FAIL */
/*==========================================================*/
esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{ 
	ESP_LOGW(TAG, "Erreur 404 URI %s non trouvé", req->uri);
  // NOTE ferme la connection pour éviter httpd_sock_err: error in send : 104 ?
  httpd_resp_set_hdr(req, "Connection", "close");
    
	httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Erreur 404 non definie");
	return ESP_FAIL;
}

/*==========================================================*/
/* envoi le fichier filepath : NE PAS OUBLIER "/" en debut d'adresse */ /* 
	retourne ESP_OK ou ESP_FAIL */
/*==========================================================*/
esp_err_t envoiFichier(httpd_req_t *req, const char *filepath)
{
// 	ESP_LOGI(TAG, "envoiFichier %s", req->uri);
	FILE *fd = NULL;
	fd = fopen(filepath, "r");

  if (!fd) {
    ESP_LOGE(TAG, "Impossible d'ouvrirle fichier : %s", filepath);
    /* Respond with 500 Internal Server Error */
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Impossible de lire le fichier");
    return ESP_FAIL;
  }
  
  /* Envoi le type des données du fichier */
	set_content_type_from_file(req, filepath);
	
  /* Récupère le pointeur vers le tampon de travail pour le stockage temporaire */
  char *buf = (( _donnee_serveur *)req->user_ctx)->tampon;
  size_t chunksize;
    
  do {
    /* Lire le fichier en morceaux dans le tampon de travail */
    chunksize = fread(buf, 1, TAILLE_TAMPON, fd);
    if (chunksize > 0) {
      /* Envoyer le contenu du tampon sous forme de bloc de réponse HTTP */
      if (httpd_resp_send_chunk(req, buf, chunksize) != ESP_OK) {
        fclose(fd);
        ESP_LOGE(TAG, "L'envoi du fichier [%i octets] %s a échoué ! ", chunksize, filepath);
        /* Annuler l'envoi du fichier */
        httpd_resp_sendstr_chunk(req, NULL);
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Échec de l'envoi du fichier");
				return ESP_FAIL;
    	}
    }

    /* Continuer à boucler jusqu'à ce que tout le fichier soit envoyé */
  } while (chunksize != 0);
  fclose(fd);
  /* Envoie un morceau vide pour signaler la fin de la réponse HTTP */
//   httpd_resp_sendstr_chunk(req, NULL);

    // NOTE ferme la connection pour éviter httpd_sock_err: error in send : 104 ?
  httpd_resp_set_hdr(req, "Connection", "close");
    
  httpd_resp_send_chunk(req, NULL, 0);
	return ESP_OK;
}

/*==========================================================*/
/* Envoi la page /index.html */ /* 
	retourne ESP_OK ou ESP_FAIL */
/*==========================================================*/
esp_err_t index_handler(httpd_req_t *req)
{
	return envoiFichier(req, "/index.html");
}

/*==========================================================*/
/* Envoi /favicon.ico */ /* 
	retourne ESP_OK ou ESP_FAIL */
/*==========================================================*/
esp_err_t favicon_get_handler(httpd_req_t *req)
{
  return envoiFichier(req, "/favicon.ico");
}

/*==========================================================*/
/* Envoi /favicon.ico */ /* 
	retourne ESP_OK ou ESP_FAIL */
/*==========================================================*/
esp_err_t envoi_fichier_handler(httpd_req_t *req)
{
  return envoiFichier(req, (const char*) req->uri);
}

/*==========================================================*/
/* Enregistre les URI des pages disponibles sur le serveur  */ /* 
	établi le lien avec les données de contexte pour l'utilisation par les gestionnaires */
/*==========================================================*/
esp_err_t enregiste_URI(httpd_handle_t server, _donnee_serveur *donnee_serveur)
{
	// NOTE NE PAS utiliser la racine /fichier/ qui est utilisé par le serveur ce fichier (serveurFichier.c)
	/* Gestionnaire d'URI pour envoyer le favicon */
	esp_err_t ret;

	httpd_uri_t favicon = {
		.uri       = "/favicon.ico",
		.method    = HTTP_GET,
		.handler   = favicon_get_handler,
		.user_ctx  = donnee_serveur
	};
	ret = httpd_register_uri_handler(server, &favicon);
	if(ret != ESP_OK)
		return ret;
	
	/* Gestionnaire d'URI pour envoyer la page index.html */
	httpd_uri_t indexSite = {
		.uri       = "/",
		.method    = HTTP_GET,
		.handler   = index_handler,
		.user_ctx  = donnee_serveur
	};
	ret = httpd_register_uri_handler(server, &indexSite);
	if(ret != ESP_OK)
		return ret;

	httpd_uri_t envoiFichiers = {
		.uri       = "/www/*",			// matche avec toutes les URI qui commence par www/
		.method    = HTTP_GET,
		.handler   = envoi_fichier_handler,
		.user_ctx  = donnee_serveur
	};
	ret = httpd_register_uri_handler(server, &envoiFichiers);
	if(ret != ESP_OK)
		return ret;

	httpd_uri_t _initPage = {
		.uri       = "/initPage",
		.method    = HTTP_GET,
		.handler   = initPageHTML,
		.user_ctx  = donnee_serveur
	};
	ret = httpd_register_uri_handler(server, &_initPage);
	if(ret != ESP_OK)
		return ret;

	httpd_uri_t _miseAJour = {
		.uri       = "/miseAJour",
		.method    = HTTP_POST,
		.handler   = miseAJour,
		.user_ctx  = donnee_serveur
	};
	ret = httpd_register_uri_handler(server, &_miseAJour);
	if(ret != ESP_OK)
		return ret;

	httpd_uri_t _renvoiEtat = {
		.uri       = "/renvoiEtat",
		.method    = HTTP_GET,
		.handler   = renvoiEtat,
		.user_ctx  = donnee_serveur
	};
	ret = httpd_register_uri_handler(server, &_renvoiEtat);
	if(ret != ESP_OK)
		return ret;

	httpd_uri_t _infoDBG = {
		.uri       = "/infoDBG",
		.method    = HTTP_GET,
		.handler   = infoDBG,
		.user_ctx  = donnee_serveur
	};
	ret = httpd_register_uri_handler(server, &_infoDBG);
	if(ret != ESP_OK)
		return ret;

	/* Gestionnaire d'erreur 404 */
	ret = httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, http_404_error_handler);
	if(ret != ESP_OK)
		return ret;
	
	return ESP_OK;
}

/*==========================================================*/
/* Gestionnaire d'évènement de déconnection wifi */ /* 
	- arrête le serveur http si le wifi est déconnecté
	- arrête le serveur SNTP
	 */
/*==========================================================*/
void disconnect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
	ESP_LOGW(TAG, "Wifi déconnecté");
	httpd_handle_t* server = (httpd_handle_t*) arg;
	if (*server) {
		ESP_LOGW(TAG, "Stopping webserver");
		if (stop_webserver(*server) == ESP_OK) {
			*server = NULL;
		} else {
			ESP_LOGE(TAG, "Failed to stop http server");
		}
	}
	stop_SNTP();
}

/*==========================================================*/
/* Gestionnaire d'évènement de connection wifi établie */ /* 
	- redémarre si besoin le serveur http si le wifi est connecté
	- redémarre le serveur SNTP
	 */
/*==========================================================*/
void connect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
	ESP_LOGW(TAG, "Wifi connecté");
	httpd_handle_t* server = (httpd_handle_t*) arg;
	if (*server == NULL) {
		*server = start_webserver();
	}
	// !!! modif
// 	start_SNTP();
}

/*==========================================================*/
/* Arrete le serveur */ /*  */
/*==========================================================*/
esp_err_t stop_webserver(httpd_handle_t server)
{
	return httpd_stop(server);
}

/*==========================================================*/
/* Démarre le serveur http */ /* 
	retourne le serveur créé: server ou NULL si erreur*/
/*==========================================================*/
static httpd_handle_t start_webserver(void)
{
	httpd_handle_t server = NULL;
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	config.lru_purge_enable = true;
//   Utiliser la fonction de correspondance de URI Wildcard afin de permettre 
//   au même gestionnaire de répondre à plusieurs URI cibles différentes qui correspondent au schéma générique
	config.uri_match_fn = httpd_uri_match_wildcard;
	// NOTE : augmenter le nombre de gestionnaires d'URI si alert /
	// "httpd_uri: httpd_register_uri_handler: no slots left for registering handler"
	config.max_uri_handlers = 20;
	// NOTE La taille par défaut de la pile esp-http-server n'est que de 4K, ce qui peut entraîner un débordement de pile.
	// Pour augmenter la pile de serveurs Web ajouter 'config.stack_size=TAILLE_TAMPON' par exemple
	config.stack_size=TAILLE_TAMPON;

// 	config.max_uri_handlers  = 90;
// 	config.max_resp_headers  = 20;
// 	config.stack_size        = 1024*10;
	config.recv_wait_timeout = 30;//5 by default
	// NOTE modif send_wait_timeout
	config.send_wait_timeout = 30;//5 by default
    	
	// NOTE faut-il affecter le core 0 ?
// 	config.core_id = 0;
	
	// Start the httpd server
	if (httpd_start(&server, &config) == ESP_OK) {
			// Set URI handlers
		ESP_LOGW(TAG, "Serveur démarré port %d - core %d (priorite %d)", config.server_port, xPortGetCoreID(), config.task_priority);
		return server;
	}

	ESP_LOGW(TAG, "Error starting server!");
	return NULL;
}

/*==========================================================*/
/* Initialise le serveur et le démarre */ /*  */
/*==========================================================*/
// int initServeur(void)
int initServeur(void *structMAJ)
{
	esp_err_t ret;
// 	ESP_LOGW(TAG,"initServeur");

	static httpd_handle_t server = NULL;
	static _donnee_serveur *donnee_serveur; // = NULL;
  /* Allouer de la mémoire pour les données du serveur */
  donnee_serveur = malloc(sizeof(_donnee_serveur));
	if(!donnee_serveur) {    // Si l'allocation a échoué.
		ESP_LOGW(TAG,"l'allocation a échoué");
		return ESP_FAIL;
	}	
	mutexInterface = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    
	// teste si le wifi est connecté
	if(info_wifi() == ESP_OK) {
		/* Start the server for the first time */
		server = start_webserver();
		if(server) {
			ret = enregiste_URI(server, donnee_serveur);
			if(ret != ESP_OK) {
				ESP_LOGE(TAG, "Erreur[%i] serveur enregiste_URI", ret);
				return ESP_FAIL;	
			}
		}
		else {
			ESP_LOGW(TAG, "Erreur démarrage du serveur");
			return ESP_FAIL;	
		}

	// Enregistrer les gestionnaires d'événements pour arrêter le serveur lorsque le Wi-Fi  est déconnecté,
	// et le redémarrer dès la connexion établie.
		ret = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server);
		if(ret != ESP_OK) {
			goto 	erreur_init_serveur;
		}
		
		ret = esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server);
		if(ret != ESP_OK) {
			goto 	erreur_init_serveur;
		}
		
		donnee_serveur->structMAJ = structMAJ;

			/* initialise le serveur de fichier */
		ret = initServeurFichier(server, donnee_serveur->tampon);
		if(ret != ESP_OK) {
			goto 	erreur_init_serveur;
		}
		
		ESP_LOGI(TAG, "Init serveur OK");
		return ESP_OK;
	}
	else {
		goto 	erreur_init_serveur;
	}

erreur_init_serveur:
	ESP_LOGE(TAG, "ERREUR init serveur");
	return ESP_FAIL;	
}

