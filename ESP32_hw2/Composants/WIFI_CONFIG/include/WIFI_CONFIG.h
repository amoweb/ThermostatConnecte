#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

#define WIFI_CONNECTED_BIT 	BIT0
#define WIFI_FAIL_BIT      	BIT1
#define WIFI_RUN_ETAT_BIT		BIT2
#define WIFI_STOP_ETAT_BIT	BIT3

int init_WIFI(void);
int info_wifi(void);

#if(CONFIG_TACHE_ETAT_WIFI)
void stop_etat_wifi(void);
void run_etat_wifi(void);
#endif

#endif // WIFI_CONFIG_H