// initialise la mise à l'heure via un serveur SNTP

#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_netif_sntp.h"
#include "lwip/ip_addr.h"
#include "esp_sntp.h"

#include  "HEURE_SNTP.h"

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* defines */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
// #define CALLBACK_SYNC

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*  constantes */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
static const char *TAG = "SNTP";

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*  variables externes */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*  variables locales */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
int etat_SNTP = 0;

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*  fonctions locales */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*  fonctions globales */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

/*==========================================================*/
/*  */ /*  */
/*==========================================================*/


#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_CUSTOM
/*==========================================================*/
/* Utilisé pour un mode de synchronisation personnalisé  */ /*  */
/*==========================================================*/
void sntp_sync_time(struct timeval *tv)
{
   settimeofday(tv, NULL);
   ESP_LOGI(TAG, "Time is synchronized from custom code");
   sntp_set_sync_status(SNTP_SYNC_STATUS_COMPLETED);
}
#endif

#ifdef CALLBACK_SYNC
/*==========================================================*/
/* callback de synchronisation */ /*  */
/*==========================================================*/
void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Notification of a time synchronization event");
}
#endif

/*==========================================================*/
/* Synchronise date et heure suivant la time zone */ /* 
	- démarre la connection au serveur SNTP s'il n'y pas eu de synchro précédemment */
/*==========================================================*/
void maj_heure(void)
{
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    // Is time set? If not, tm_year will be (1970 - 1900).
    if (timeinfo.tm_year < (2016 - 1900)) {
//         ESP_LOGI(TAG, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
        start_SNTP();
        // update 'now' variable with current time
        time(&now);
    }
#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_SMOOTH
    else {
        // add 500 ms error to the current system time.
        // Only to demonstrate a work of adjusting method!
        {
            ESP_LOGI(TAG, "Add a error for test adjtime");
            struct timeval tv_now;
            gettimeofday(&tv_now, NULL);
            int64_t cpu_time = (int64_t)tv_now.tv_sec * 1000000L + (int64_t)tv_now.tv_usec;
            int64_t error_time = cpu_time + 500 * 1000L;
            struct timeval tv_error = { 
            	.tv_sec = error_time / 1000000L, 
            	.tv_usec = error_time % 1000000L };
            settimeofday(&tv_error, NULL);
        }

        ESP_LOGI(TAG, "Time was set, now just adjusting it. Use SMOOTH SYNC method.");
        start_SNTP();
        // update 'now' variable with current time
        time(&now);
    }
#endif

    char strftime_buf[64];
    if (sntp_get_sync_mode() == SNTP_SYNC_MODE_SMOOTH) {
        struct timeval outdelta;
        while (sntp_get_sync_status() == SNTP_SYNC_STATUS_IN_PROGRESS) {
            adjtime(NULL, &outdelta);
//             ESP_LOGI(TAG, "Waiting for adjusting time ... outdelta = %jd sec: %li ms: %li us",
//                         (intmax_t)outdelta.tv_sec,
//                         outdelta.tv_usec/1000,
//                         outdelta.tv_usec%1000);
            vTaskDelay(2000 / portTICK_PERIOD_MS);
        }
    }
    // Set timezone pour Saulce sur Rhone
//     setenv("TZ", "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", 1);
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
    tzset();
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "Date et heure courante à Saulce sur Rhone : %s", strftime_buf);
}

/*==========================================================*/
/* Affiche les serveurs NTP */ /*  */
/*==========================================================*/
/*
static void print_servers(void)
{
    ESP_LOGI(TAG, "List of configured NTP servers:");

    for (uint8_t i = 0; i < SNTP_MAX_SERVERS; ++i){
        if (esp_sntp_getservername(i)){
            ESP_LOGI(TAG, "server %d: %s", i, esp_sntp_getservername(i));
        } else {
            // we have either IPv4 or IPv6 address, let's print it
            char buff[INET6_ADDRSTRLEN];
            ip_addr_t const *ip = esp_sntp_getserver(i);
            if (ipaddr_ntoa_r(ip, buff, INET6_ADDRSTRLEN) != NULL)
                ESP_LOGI(TAG, "server %d: %s", i, buff);
        }
    }
}
*/

/*==========================================================*/
/* Arrete l'accès au serveur SNTP */ /* 
	Utilise le flag : etat_SNTP */
/*==========================================================*/
void stop_SNTP(void) {
	if(etat_SNTP) {
		esp_netif_sntp_deinit();
		etat_SNTP = 0;
	}
}

/*==========================================================*/
/* Démarre l'accès au serveur SNTP */ /*  */
/*==========================================================*/
void start_SNTP(void)
{
	if(etat_SNTP){
		return;
	}
	etat_SNTP = 1;
	
// 	ESP_LOGI(TAG, "Initializing and starting SNTP");
#if CONFIG_LWIP_SNTP_MAX_SERVERS > 1
	/* This demonstrates configuring more than one server */
	esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG_MULTIPLE(2,
														 ESP_SNTP_SERVER_LIST(CONFIG_SNTP_TIME_SERVER, "pool.ntp.org" ) );
#else /* !CONFIG_LWIP_SNTP_MAX_SERVERS */
	/* This is the basic default config with one server and starting the service */
	esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG(CONFIG_SNTP_TIME_SERVER);
#endif	/* CONFIG_LWIP_SNTP_MAX_SERVERS */

#ifdef CALLBACK_SYNC
	config.sync_cb = time_sync_notification_cb;     // Note: This is only needed if we want
#endif
   
#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_SMOOTH
	config.smooth_sync = true;
#endif

	esp_netif_sntp_init(&config);

/*
	print_servers();
*/

	// wait for time to be set
	time_t now = 0;
	struct tm timeinfo = { 0 };
	int retry = 0;
	const int retry_count = 15;
	while (esp_netif_sntp_sync_wait(2000 / portTICK_PERIOD_MS) == ESP_ERR_TIMEOUT && ++retry < retry_count) {
			ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
	}
	time(&now);
	localtime_r(&now, &timeinfo);
}

/*==========================================================*/
/* Init serveur SNTP */ /*  */
/*==========================================================*/
	/* NOTE: This call should be made BEFORE esp acquires IP address from DHCP,
     * otherwise NTP option would be rejected by default.
  */
void init_SNTP(void) {
    ESP_LOGI(TAG, "Initializing SNTP");
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG(CONFIG_SNTP_TIME_SERVER);
    config.start = false;                       // start SNTP service explicitly (after connecting)
    config.server_from_dhcp = true;             // accept NTP offers from DHCP server, if any (need to enable *before* connecting)
    config.renew_servers_after_new_IP = true;   // let esp-netif update configured SNTP server(s) after receiving DHCP lease
    config.index_of_first_server = 1;           // updates from server num 1, leaving server 0 (from DHCP) intact
    // configure the event on which we renew servers
    config.ip_event_to_renew = IP_EVENT_STA_GOT_IP;
//     config.sync_cb = time_sync_notification_cb; // only if we need the notification function
    esp_netif_sntp_init(&config);
}  

/*==========================================================*/
/* synchronise horloge avec serveur SNTP */ /*  */
/*==========================================================*/
void sync_SNTP(void) {
	time_t now = 0;
	struct tm timeinfo = { 0 };
	int retry = 0;
	const int retry_count = 15;
	
	esp_netif_sntp_start();
// wait for time to be set
	while (esp_netif_sntp_sync_wait(2000 / portTICK_PERIOD_MS) == ESP_ERR_TIMEOUT && ++retry < retry_count) {
			ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
	}
	time(&now);
	localtime_r(&now, &timeinfo);
	esp_sntp_stop();
}
/*==========================================================*/
/* Synchronise date et heure suivant la time zone */ /* 
	- démarre la connection au serveur SNTP s'il n'y pas eu de synchro précédemment */
/*==========================================================*/
void maj_Heure(void)
{
    char strftime_buf[64];
    time_t now;
    struct tm timeinfo;

    sync_SNTP();
    time(&now);

    // Set timezone pour Saulce sur Rhone
//     setenv("TZ", "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", 1);
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
    tzset();
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "Date et heure courante à Saulce sur Rhone : %s", strftime_buf);
}

