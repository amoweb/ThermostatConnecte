#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <dirent.h>

#include "esp_err.h"
#include "esp_log.h"

#include "esp_vfs.h"
#include "esp_spiffs.h"
#include "esp_http_server.h"

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* defines */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
// NOTE la taille du tampon doit être égale à celle du serveur http
#define TAILLE_TAMPON  8192

/* taille max du non de fichier (ESP_VFS_PATH_MAX = 15, defini dans esp_vsf.h) */
#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + CONFIG_SPIFFS_OBJ_NAME_LEN)

/* Max size of an individual file. Make sure this value is same as that set in upload_script.html */
#define MAX_FILE_SIZE     (200*1024) // 200 KB
#define MAX_FILE_SIZE_STR "200KB"

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*  constantes */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
static const char *TAG = "Serveur de fichier";

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*  variables externes */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*  variables locales */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*  fonctions locales */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*  fonctions globales */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

/*==========================================================*/
/*  */ /*  */
/*==========================================================*/


/*==========================================================*/
/* Envoie la page HTML pour gérer les fichier du SPIFF */ /* 
	Envoie une réponse HTTP avec un HTML généré composé d'une liste 
	de tous les fichiers et dossiers sous le chemin demandé.
	Avec SPIFFS, cela renvoie la liste vide lorsque le chemin est une chaîne autre 
	que «/», puisque Spiffs ne prend pas en charge les répertoires
	NOTE : nécessite le fichier 'script.html' */
/*==========================================================*/
static esp_err_t http_resp_dir_html(httpd_req_t *req, const char *dirpath)
{
  char entrypath[FILE_PATH_MAX];
  char entrysize[16];
  const char *entrytype;
	
	DIR *dir;
  struct dirent *entry;
  struct stat entry_stat;

  const char *uri;
  uri = req->uri;
	if(strstr(uri, "/"CONFIG_DOSSIER_DE_BASE) != NULL) {
 		uri += strlen("/"CONFIG_DOSSIER_DE_BASE);
	}

  if (dirpath[strlen(dirpath) - 1] == '/') {
		/* supprimer de / de fin de chamin */
  	char dossier[FILE_PATH_MAX];
		strlcpy(dossier, dirpath, strlen(dirpath));
  	dir = opendir(dossier);
		ESP_LOGI(TAG, "[%s] opendir dossier : %s résultat %s", __FUNCTION__, dossier, dir==NULL ? "erreur" : "OK");
  }
  else {
  	dir = opendir(dirpath);
		ESP_LOGI(TAG, "[%s] opendir dirpath : %s résultat %s", __FUNCTION__, dirpath, dir==NULL ? "erreur" : "OK");
  }

  const size_t dirpath_len = strlen(dirpath);

  /* Récupérer le chemin de base du stockage de fichiers pour construire le chemin complet */
  strlcpy(entrypath, dirpath, sizeof(entrypath));

  if (!dir) {
    ESP_LOGE(TAG, "Le dossier %s n'existe pas", dirpath);
    /* Respond with 404 Not Found */
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Le dossier n'existe pas");
    return ESP_FAIL;
  }

  /* Get handle to embedded file upload script */
  extern const unsigned char script_start[] asm("_binary_script_html_start");
  extern const unsigned char script_end[]   asm("_binary_script_html_end");
  const size_t script_size = (script_end - script_start);

  /* Ajoute un formulaire de téléchargement de fichier et un script qui, lors de l'exécution, envoie une requête POST à /upload */
  httpd_resp_send_chunk(req, (const char *)script_start, script_size);

  /* Itérer sur tous les fichiers/dossiers et récupérer leurs noms et tailles */
  while ((entry = readdir(dir)) != NULL) {
    entrytype = (entry->d_type == DT_DIR ? "dossier" : "fichier");

    strlcpy(entrypath + dirpath_len, entry->d_name, sizeof(entrypath) - dirpath_len);
    if (stat(entrypath, &entry_stat) == -1) {
      ESP_LOGE(TAG, "Failed to stat %s : %s", entrytype, entry->d_name);
      continue;
    }
    sprintf(entrysize, "%ld", entry_stat.st_size);
    ESP_LOGI(TAG, "Trouvé %s : %s (%s bytes)", entrytype, entry->d_name, entrysize);

    /* Envoyer un morceau de fichier HTML contenant des entrées de table avec le nom et la taille du fichier */
    httpd_resp_sendstr_chunk(req, "<tr><td><a href=\"");
    httpd_resp_sendstr_chunk(req, req->uri);
    httpd_resp_sendstr_chunk(req, entry->d_name);
    if (entry->d_type == DT_DIR) {
      httpd_resp_sendstr_chunk(req, "/");
    }
    httpd_resp_sendstr_chunk(req, "\">");
    httpd_resp_sendstr_chunk(req, entry->d_name);
    httpd_resp_sendstr_chunk(req, "</a></td><td>");
    httpd_resp_sendstr_chunk(req, entrytype);
    httpd_resp_sendstr_chunk(req, "</td><td>");
    httpd_resp_sendstr_chunk(req, entrysize);
    httpd_resp_sendstr_chunk(req, "</td><td>");
    httpd_resp_sendstr_chunk(req, "<form method=\"post\" action=\"/delete");
    httpd_resp_sendstr_chunk(req, req->uri);
    httpd_resp_sendstr_chunk(req, entry->d_name);
    httpd_resp_sendstr_chunk(req, "\"><button type=\"submit\">Supprimer</button></form>");
    httpd_resp_sendstr_chunk(req, "</td></tr>\n");
  }
  closedir(dir);

  /* Terminer le tableau de la liste des fichiers */
  httpd_resp_sendstr_chunk(req, "</tbody></table>");

  size_t total = 0, used = 0;
//   ret = esp_spiffs_info(NULL, &total, &used);
  if (esp_spiffs_info(NULL, &total, &used) == ESP_OK) {
		char dispo[50];
		sprintf(dispo,"Espace disponible : %i kbytes", (total-used)/1024);
		httpd_resp_sendstr_chunk(req, dispo);
  }

  /* Envoyez le morceau restant du fichier HTML pour le compléter */
  httpd_resp_sendstr_chunk(req, "</body></html>");

  /* Envoie un morceau vide pour signaler la fin de la réponse HTTP */
  httpd_resp_sendstr_chunk(req, NULL);
  return ESP_OK;
}

/*==========================================================*/
/*  Définir le type de contenu de la réponse HTTP en fonction de l'extension de fichier */ /*  */
/*==========================================================*/
#define IS_FILE_EXT(filename, ext) (strcasecmp(&filename[strlen(filename) - sizeof(ext) + 1], ext) == 0)
esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filename)
{
  if (IS_FILE_EXT(filename, ".pdf")) {
    return httpd_resp_set_type(req, "application/pdf");
  } else if (IS_FILE_EXT(filename, ".html")) {
    return httpd_resp_set_type(req, "text/html");
  } else if (IS_FILE_EXT(filename, ".jpeg")) {
    return httpd_resp_set_type(req, "image/jpeg");
  } else if (IS_FILE_EXT(filename, ".ico")) {
    return httpd_resp_set_type(req, "image/x-icon");
  } else if (IS_FILE_EXT(filename, ".css")) {
    return httpd_resp_set_type(req, "text/css");
  } else if (IS_FILE_EXT(filename, ".js")) {
    return httpd_resp_set_type(req, "text/javascript");
  }
  /* Il s'agit d'un ensemble limité uniquement */
	/* Pour tout autre type, toujours défini en texte brut */
  return httpd_resp_set_type(req, "text/plain");
}

/*==========================================================*/
/* Copie le chemin complet dans le tampon de destination  */ /* 
		renvoie un pointeur vers le chemin (en sautant le chemin de base précédent) */
/*==========================================================*/
static const char* get_path_from_uri(char *dest, const char *uri, size_t destsize)
{
  size_t pathlen = strlen(uri);
  
  size_t longPrefixUri = 0;
	if(strstr(uri, "/"CONFIG_DOSSIER_DE_BASE) != NULL) {
 		longPrefixUri = strlen("/"CONFIG_DOSSIER_DE_BASE);
	}

// 	ESP_LOGI(TAG,"[%s][%i] uri [%s][%i] longPrefixUri [%s][%i] destsize[%i]", 
// 																					__FUNCTION__, __LINE__
// 	 																				,uri, pathlen
// 	 																				,("/"CONFIG_DOSSIER_DE_BASE), longPrefixUri
// 	 																				,destsize
// 	 																				);

  const char *quest = strchr(uri, '?');
  if (quest) {
    pathlen = MIN(pathlen, quest - uri);
  }
  const char *hash = strchr(uri, '#');
  if (hash) {
    pathlen = MIN(pathlen, hash - uri);
  }

  if (pathlen + 1 > destsize) {
    /* La chaîne de chemin complet ne rentre pas dans le tampon de destination */
    ESP_LOGI(TAG, "chemin de fichier trop long");
    return NULL;
  }

    // construit le chemin complet (base + path) en supprimant la base de la requête URL (CONFIG_DOSSIER_DE_BASE)
  strlcpy(dest, uri+longPrefixUri, pathlen + 1);
//  	ESP_LOGI(TAG,"[%s][%i] dest [%s]", __FUNCTION__, __LINE__, dest);
   return dest;
}

/*==========================================================*/
/* Gestionnaire pour télécharger un fichier hébergé sur le serveur ESP32 */ /*  */
/*==========================================================*/
esp_err_t download_get_handler(httpd_req_t *req)
{
  char filepath[FILE_PATH_MAX];
  FILE *fd = NULL;
  struct stat file_stat;

  const char *filename = get_path_from_uri(filepath, req->uri, sizeof(filepath));

  ESP_LOGI(TAG, "[%s] req->uri [%s] filepath [%s] filename [%s]", __FUNCTION__, req->uri, filepath, filename);
  
  if (!filename) {
    ESP_LOGE(TAG, "Le nom de fichier est trop long");
    /* Respond with 500 Internal Server Error */
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Le nom de fichier est trop long");
    return ESP_FAIL;
  }

  /* Si le nom a un '/' final, répondre avec le contenu du répertoire */
  if (filename[strlen(filename) - 1] == '/') {
    return http_resp_dir_html(req, filepath);
  }

  if (stat(filepath, &file_stat) == -1) {
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "le fichier n'existe pas");
    return ESP_FAIL;
  }

  fd = fopen(filepath, "r");
  if (!fd) {
    ESP_LOGE(TAG, "Impossible de lire le fichier existant : %s", filepath);
    /* Respond with 500 Internal Server Error */
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Impossible de lire le fichier existant");
    return ESP_FAIL;
  }

  ESP_LOGI(TAG, "Envoie du fichier : %s (%ld bytes)...", filename, file_stat.st_size);
  set_content_type_from_file(req, filename);

  /* Récupère le pointeur vers le tampon de travail pour le stockage temporaire */
  char *chunk = req->user_ctx;
  size_t chunksize;
  do {
    /* Lire le fichier en morceaux dans le tampon de travail */
    chunksize = fread(chunk, 1, TAILLE_TAMPON, fd);

    if (chunksize > 0) {
      /* Envoyer le contenu du tampon sous forme de bloc de réponse HTTP */
      if (httpd_resp_send_chunk(req, chunk, chunksize) != ESP_OK) {
        fclose(fd);
        ESP_LOGE(TAG, "L'envoi du fichier a échoué !");
        /* Annuler l'envoi du fichier */
        httpd_resp_sendstr_chunk(req, NULL);
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Échec de l'envoi du fichier");
         return ESP_FAIL;
       }
    }

    /* Continuer à boucler jusqu'à ce que tout le fichier soit envoyé */
  } while (chunksize != 0);

  /* Fermer le fichier après l'envoi terminé */
  fclose(fd);
  ESP_LOGI(TAG, "Envoi du fichier terminé");

  /* Répondre avec un bloc vide pour signaler la fin de la réponse HTTP */
#ifdef CONFIG_HTTPD_CONN_CLOSE_HEADER
  httpd_resp_set_hdr(req, "Connection", "close");
#endif
  httpd_resp_send_chunk(req, NULL, 0);
  return ESP_OK;
}

/*==========================================================*/
/* Gestionnaire pour charger un fichier sur le serveur */ /*  */
/*==========================================================*/
esp_err_t upload_post_handler(httpd_req_t *req)
{
  char filepath[FILE_PATH_MAX];
  FILE *fd = NULL;
  struct stat file_stat;

  /* Ignorer le premier '/ upload' de l'URI pour obtenir le nom du fichier */
	/* Notez que sizeof() compte la terminaison NULL d'où le -1 */
  const char *filename = get_path_from_uri(filepath, req->uri + sizeof("/upload") - 1, sizeof(filepath));
	ESP_LOGI(TAG, "Nom de fichier : %s filepath : %s", filename, filepath);
  if (!filename) {
    /* Répondre avec une erreur de serveur interne 500 */
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Nom de fichier trop long");
    return ESP_FAIL;
  }

  /* Filename cannot have a trailing '/' */
  if (filename[strlen(filename) - 1] == '/') {
    ESP_LOGE(TAG, "Nom de fichier non valide : %s", filename);
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Nom de fichier non valide");
    return ESP_FAIL;
  }

  if (stat(filepath, &file_stat) == 0) {
    ESP_LOGE(TAG, "Le fichier existe déjà : %s", filepath);
    /* Respond with 400 Bad Request */
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Le fichier existe déjà");
    return ESP_FAIL;
  }

  /* Le fichier ne peut pas dépasser une limite */
  if (req->content_len > MAX_FILE_SIZE) {
    ESP_LOGE(TAG, "Fichier trop large : %d bytes", req->content_len);
    /* Respond with 400 Bad Request */
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "La taille du fichier doit être inférieure à " MAX_FILE_SIZE_STR "!");
    /* Renvoie l'échec de la fermeture de la connexion sous-jacente sinon
		* le contenu du fichier entrant gardera le socket occupé */
    return ESP_FAIL;
  }

  fd = fopen(filepath, "w");
  if (!fd) {
    ESP_LOGE(TAG, "La création du fichier a échoué : %s", filepath);
    /* Respond with 500 Internal Server Error */
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "La création du fichier a échoué");
    return ESP_FAIL;
  }

  ESP_LOGI(TAG, "Réception du fichier : %s...", filename);

  /* Récupérer le pointeur vers le tampon de travail pour le stockage temporaire */
  char *buf = req->user_ctx;
  int received;

  /* La longueur du contenu de la requête donne la taille du fichier téléchargé */
  int remaining = req->content_len;

  while (remaining > 0) {

    ESP_LOGI(TAG, "Données restantes : %d", remaining);
    /* Recevoir le fichier partie par partie dans un tampon */
    if ((received = httpd_req_recv(req, buf, MIN(remaining, TAILLE_TAMPON))) <= 0) {
      if (received == HTTPD_SOCK_ERR_TIMEOUT) {
        /* Réessayer si le délai d'attente s'est produit */
        continue;
      }

      /* En cas d'erreur irrécupérable, fermer et supprimer le fichier inachevé */
      fclose(fd);
      unlink(filepath);

      ESP_LOGE(TAG, "La réception du fichier a échoué!");
      /* Respond with 500 Internal Server Error */
      httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "La réception du fichier a échoué!");
      return ESP_FAIL;
    }

    /* Écrire le contenu du tampon dans un fichier sur le serveur */
    if (received && (received != fwrite(buf, 1, received, fd))) {
      /* Impossible d'écrire tout dans le fichier! Le serveur est peut-être plein ? */
      fclose(fd);
      unlink(filepath);

      ESP_LOGE(TAG, "Échec de l'écriture du fichier!");
      /* Respond with 500 Internal Server Error */
      httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Impossible d'écrire le fichier dans le stockage");
      return ESP_FAIL;
    }

    /* Gardez une trace de la taille restante du fichier restant à uploader */
    remaining -= received;
  }

  /* Fermer le fichier une fois le téléchargement terminé */
  fclose(fd);
  ESP_LOGI(TAG, "Réception du fichier terminée");

  /* Rediriger vers root pour voir la liste de fichiers mise à jour */
  httpd_resp_set_status(req, "303 See Other");
  httpd_resp_set_hdr(req, "Location", "/"CONFIG_DOSSIER_DE_BASE"/");
#ifdef CONFIG_HTTPD_CONN_CLOSE_HEADER
  httpd_resp_set_hdr(req, "Connection", "close");
#endif
  httpd_resp_sendstr(req, "Fichier téléchargé avec succès");
  return ESP_OK;
}

/*==========================================================*/
/* Gestionnaire pour supprimer un fichier du serveur */ /*  */
/*==========================================================*/
esp_err_t delete_post_handler(httpd_req_t *req)
{
  char filepath[FILE_PATH_MAX];
  struct stat file_stat;

  /* Ignorer le premier '/delete' de l'URI pour obtenir le nom de fichier */
	/* Notez que sizeof() compte la terminaison NULL d'où le -1 */
  const char *filename = get_path_from_uri(filepath, req->uri  + sizeof("/delete") - 1, sizeof(filepath));
  if (!filename) {
    /* Respond with 500 Internal Server Error */
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Nom de fichier trop long");
    return ESP_FAIL;
  }

  /* Filename cannot have a trailing '/' */
  if (filename[strlen(filename) - 1] == '/') {
    ESP_LOGE(TAG, "Nom de fichier non valide : %s", filename);
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Nom de fichier non valide");
    return ESP_FAIL;
  }

  if (stat(filepath, &file_stat) == -1) {
    ESP_LOGE(TAG, "Fichier n'existe pas : %s", filename);
    /* Respond with 400 Bad Request */
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Fichier ne existe pas");
    return ESP_FAIL;
  }

//   ESP_LOGI(TAG, "Deleting file : %s", filename);
  /* Delete file */
  unlink(filepath);

  /* Redirect onto root to see the updated file list */
  httpd_resp_set_status(req, "303 See Other");
  httpd_resp_set_hdr(req, "Location", "/"CONFIG_DOSSIER_DE_BASE"/");
#ifdef CONFIG_HTTPD_CONN_CLOSE_HEADER
  httpd_resp_set_hdr(req, "Connection", "close");
#endif
  httpd_resp_sendstr(req, "Fichier supprimé avec succès");
  return ESP_OK;
}

/*==========================================================*/
/* Fonction pour démarrer le serveur de fichiers */ /*  */
/*==========================================================*/
esp_err_t initServeurFichier(httpd_handle_t server, char *data_serveur_fichier)
{
  /* Gestionnaire d'URI pour obtenir les fichiers téléchargés */
  httpd_uri_t file_download = {
    .uri     = "/"CONFIG_DOSSIER_DE_BASE"/*",  // Correspond à tous les URI de type fichier/path/to/file
    .method  = HTTP_GET,
    .handler   = download_get_handler,
    .user_ctx  = data_serveur_fichier  // Passer les données du serveur comme contexte
  };
  httpd_register_uri_handler(server, &file_download);

  /* Gestionnaire d'URI pour télécharger des fichiers sur le serveur */
  httpd_uri_t file_upload = {
    .uri     = "/upload/*",   // Correspond à tous les URI de type /upload/path/to/file
    .method  = HTTP_POST,
    .handler   = upload_post_handler,
    .user_ctx  = data_serveur_fichier  // Passer les données du serveur comme contexte
  };
  httpd_register_uri_handler(server, &file_upload);

  /* Gestionnaire d'URI pour supprimer des fichiers du serveur */
  httpd_uri_t file_delete = {
    .uri     = "/delete/*",   // Correspond à tous les URI de type /delete/path/to/file
    .method  = HTTP_POST,
    .handler   = delete_post_handler,
    .user_ctx  = data_serveur_fichier  // Passer les données du serveur comme contexte
  };
  httpd_register_uri_handler(server, &file_delete);

  return ESP_OK;
}
