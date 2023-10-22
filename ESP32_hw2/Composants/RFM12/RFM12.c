
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#if defined(__linux__) || OSX
	#include "interface_materiel.h"
	#include <time.h>
	#include "utilitaire.h"
#else		// EPS32
	#include "freertos/FreeRTOS.h"
	#include "freertos/task.h"
	#include "driver/gpio.h"
	#include "freertos/queue.h"
	#include <sys/param.h>
	#include "esp_log.h"
	#include "bus_spi.h"
	#define INFOE(format, ...);
	#define INFOA(format, ...);
	#define INFOD(format, ...);
	#define INFOL(format, ...);
	#define INFO(format, ...);
#endif
#include "RFM12.h"

#ifdef RFM12B
#warning "RFM12 type B 3,3V max"
#endif

#if (FREQ_0 == 433)
#warning "frequence cenrale 433 MHz"
#elif (FREQ_0 == 868)
#warning "frequence cenrale 868 MHz"
#elif (FREQ_0 == 915)
#warning "frequence cenrale 915 MHz"
#else
#error "Frequence centrale non conforme: 433, 868 ou 915"
#endif


//       PRECAUTIONS IMPORTANTES !
// 1/ Le temps de lecture de FIFO entre chaque donnee ne doit pas exceder le temps de reception
// du recepteur, sinon il y risque de desynchronisation et necessite de renvoyer preamble
// 2/ accorder la largeur de bande du recepteur (bandwidth RFM12_CMD_RX_CTL) avec la vitesse de 
// transmission suivant tableau
// 3/ accorder la deviation de frequence de l'emetteur (modulation RFM12_CMD_TX_CTL) avec la 
// vitesse de transmission suivant tableau
//===========================================================================
//                             DEFINITIONS                                  =
//===========================================================================
	#ifndef CONFIG_GPIO_IRQ_RFM12
	#warning	"GPIO pour IT RFM 12 non défini => 27 par defaut pour carte perso"
	#define CONFIG_GPIO_IRQ_RFM12	27
	#endif

//===========================================================================
//                             VARIABLES                                    =
//===========================================================================

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* Macros */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*  constantes */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
#if defined(__linux__) || OSX
#else
static const char *TAG = "RFM12";
#endif
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*  variables externes */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*  variables locales */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
#if defined OSX
uchar dataRFM12[SIZE_FIFO] = {0};
int idx;
#endif

#if defined(__linux__) || OSX	
struct timespec start, finish, delta;
// #warning "defined __linux__  || OSX"
#else
TaskHandle_t RFMIT_HandlerTask;
#endif

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*  fonctions locales */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
#if defined(__linux__) || OSX
int lectFIFO(uchar *buf);
#endif

void RF12_CMD(int cmd);
int RFM_status(int nbBcl);
// void RFM12_reset_FIFO(void);
void RF12_WrByte(char octet);
void RF12_recept_ES(void);
void init_IT_RFM12(void *E_R_RFM12);

//===========================================================================
//                   Sous programmes                                        =
//===========================================================================
#if defined(__linux__) || OSX	
	#define	NS_PER_SECOND	1000000000
// programme pour test sur OSX et RPI
void sub_timespec(struct timespec t1, struct timespec t2, struct timespec *td)
{
    td->tv_nsec = t2.tv_nsec - t1.tv_nsec;
    td->tv_sec  = t2.tv_sec - t1.tv_sec;
    if (td->tv_sec > 0 && td->tv_nsec < 0) {
        td->tv_nsec += NS_PER_SECOND;
        td->tv_sec--;
    }
    else if (td->tv_sec < 0 && td->tv_nsec > 0) {
        td->tv_nsec -= NS_PER_SECOND;
        td->tv_sec++;
    }
}
#define START	clock_gettime(CLOCK_REALTIME, &start);
#define END		clock_gettime(CLOCK_REALTIME, &finish);
#define DELTA	sub_timespec(start, finish, &delta); \
							printf("\tDelta %d.%.6i\n", (int)delta.tv_sec, (int)(delta.tv_nsec/1000));
// 							printf("\tDelta %d.%.9ld \n", (int)delta.tv_sec, delta.tv_nsec);
#else
#define START
#define END
#define DELTA
#endif

////////////////////////////////////////////////////////////////////////
//                         INITIALISATION
////////////////////////////////////////////////////////////////////////
/*==========================================================*/
/* Initialisation RFM12 */  /* 
		initialise :
		- le bus SPI
		- le module RFM12
		- le gestionnaire d'interruption : init_IT_RFM12 avec RFMIT_HandlerTask
 */
/*==========================================================*/
_E_R_RFM12 *Init_RF12(void)
{
#if defined(__linux__) || OSX
	init_SPI();
	sleep(1);	// laisser le temps pour initialiser SPI avant de lancer le thread
#endif

	// init module RFM12
  RF12_CMD (RFM12_CONF_OFF);	// Configuration
  RF12_CMD (RFM12_RESET);      		// Power management
  RF12_CMD (RFM12_FREQ);       		// Frequency control => fréquence réelle est fonction de la bande choisie
  RF12_CMD (RFM12_RATE);       		// Data rate
  RF12_CMD (RFM12_RX_CTL);     		// Receiver control
  RF12_CMD (RFM12_FILTER_CTL); 		// Filter control
  RF12_CMD (RFM12_FIFO_RESET); 		// FIFO and reset control
#ifdef	RFM12B
// RFM12_CMD_SYNC      0xCED4    		// Sync pattern  -> version B
	RF12_CMD (RFM12_SYNC);
#endif
  RF12_CMD (RFM12_AFC_CTL);    		// AFC read command
  RF12_CMD (RFM12_TX_CTL);     		// Tx config control command
#ifdef	RFM12B
// RF12_CMD	   (0xCC77)    		// PLL control command   -> version B
	RF12_CMD	(RFM12_PLL_CTL);
#endif
  RF12_CMD (RFM12_WAKEUP);     		// Wakeup timer command
  RF12_CMD (RFM12_DUTYCYCLE);  		// Duty cycle command
  RF12_CMD (RFM12_LOWBAT_CLK); 		// Low battery detector and CLK pin clock divider

	_E_R_RFM12 *E_R_RFM12;
	E_R_RFM12 = malloc(sizeof(_E_R_RFM12));
	memset(E_R_RFM12->bufRX, 0, SIZE_FIFO*sizeof(uchar));
	E_R_RFM12->mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
	E_R_RFM12->result = VIDE;

	// init gestion interruption ESP32
#if defined(__linux__) || OSX
#else
	E_R_RFM12->irq_RFM = CONFIG_GPIO_IRQ_RFM12;
	E_R_RFM12->IT_queue = xQueueCreate(SIZE_FIFO * 3, sizeof( uint32_t ));

	// NOTE utiliser tskIDLE_PRIORITY pour éviter l'appel watchdog, mais utilise tout le temps processeur !
	xTaskCreatePinnedToCore(init_IT_RFM12, "thread_init_IT_RFM12", 10000, E_R_RFM12, 8, &RFMIT_HandlerTask, 1);
	vTaskDelay(100 / portTICK_PERIOD_MS);
#endif
	return E_R_RFM12;
}

////////////////////////////////////////////////////////////////////////
//                          EMISSION                                   /
////////////////////////////////////////////////////////////////////////
 
/*==========================================================*/
/* envoi message sur RFM12 : envoi nbre d'octets + donnees + le checksum */  /* 
		le checksum sur un octet est la somme : nbre d'octets + donnees
 */
/*==========================================================*/
void envoiMessRFM12(char *data) {
	int i = 0;
	int nbrOctet = SIZE_DATA;
  uint CheckSum  = 0; // ChkSum=0

//								!!! temps max démarrage xtal = 7 ms
//								NOTE : faut-il arrêter xtal ?
	RF12_CMD (RFM12_RESET);					// et = 0	: arret émetteur et recepteur
	RF12_CMD (RFM12_CONF_REG_ON);		// el = 1
  RF12_CMD(RFM12_EMIS);						// et = 1, er = 0	: marche émetteur
#if defined(__linux__) || OSX	
	msleep(7);
#else		// EPS32
	vTaskDelay(7/portTICK_PERIOD_MS);
#endif
      // NOTE Prevent the real time kernel swapping out the task.
      vTaskSuspendAll ();
//---------------- envoi preamble (0xAA)
  RF12_WrByte( 0xAA);
  RF12_WrByte( 0xAA);
//---------------- envoi sync word 0x2DD4
  RF12_WrByte( 0x2D );  //
  RF12_WrByte( 0xD4 );  //
//---------------- envoi id reseau
	RF12_WrByte(RESEAU);
//---------------- envoi nbre d'octets
	RF12_WrByte((uchar)nbrOctet);
	CheckSum += (uchar)nbrOctet;
	#if OSX													// NOTE simu OSX
	dataRFM12[0] = (uchar)nbrOctet;
	#endif
//---------------- envoi donnees
	for(i=0; i < SIZE_DATA; i++) {
		RF12_WrByte(data[i]);
		CheckSum += (int)data[i];
	#if OSX													// NOTE simu OSX
		dataRFM12[i+1] = data[i];
	#endif
	}
	#if defined(__linux__) || OSX
	printf("\t RF12_Emis[%i] reseau %i - %i data ", __LINE__, RESEAU, nbrOctet);
	for(int i=0; (i<SIZE_DATA); i++) {
		printf("0x%02X ", data[i]);
	}
	printf("\n");
	#endif
//---------------- envoi CheckSum
	#if OSX														// NOTE simu OSX
	dataRFM12[i+1] = (uchar)CheckSum;
	#endif
  RF12_WrByte(CheckSum);
//---------------- Nécessaire : transmit 2 dummy bytes to avoid that last bytes of real payload don't
//                 get transmitted properly (due to transmitter disabled to early)
  RF12_WrByte( 0xAA);	// DUMMY BITS
  RF12_WrByte( 0xAA);	// DUMMY BITS
      // NOTE The operation is complete.  Restart the kernel.
      xTaskResumeAll ();
      
	vTaskDelay(50/portTICK_PERIOD_MS);

      
//---------------- retour en mode réception
	RF12_recept_ES();
INFO("Fin EMISSION");
}

////////////////////////////////////////////////////////////////////////
//                         communication RFM12
////////////////////////////////////////////////////////////////////////
/*==========================================================*/
/*	envoi une commande */  /*	
		écrit 16 bits sur SPI   */
/*==========================================================*/
void RF12_CMD(int cmd) {
	uchar buf[2];
	buf[0] = cmd >> 8;
	buf[1] = cmd & 0xFF;
	write_SPI16(buf);
}

//=========================================================
/* Attente que RFM12 soit pret : SDO = 1 */  /*
	- si nbBcl = 0, effectue 1500 max et retourne 0 si RFM12 n'est pas prêt
	- sinon effectue uniquement nbBcl
 */
//=========================================================
int RFM_status(int nbBcl) {
	int nbTest = 1500;
	if(nbBcl != 0) {
		nbTest = nbBcl;
	}
	uchar buf[2];
	do {
		buf[0] = '\0';
		buf[1] = '\0';
		write_SPI16(buf);
		if(buf[0] & isRGITFFIT) {
INFO("%i boucle test status buf[0] 0x%02X%02X", 1500 - nbTest, buf[0], buf[1]);
			return 1;
		}
  	nbTest--;
  } while(nbTest);
// INFOE("RFM non pret buf[0] 0x%02X", buf[0]);
	return 0;
}

//=========================================================
/* envoi 0xB800 | RF_Data  : 2 octets */  /* */
//=========================================================
void RF12_WrByte(char octet) {
	uchar buf[2];
	if(RFM_status(0)) {
		buf[0] = (char)(RFM12_WRITE >> 8);
		buf[1] = octet;
		write_SPI16(buf);
		return;
	}
INFOE("ERREUR RF12_WrByte");
}

/*==========================================================*/
/* RAZ IT lecture de FIFO pour remise à zéro IT */  /*  */
// NOTE il est impératif de "razer" les IT générées par RFM12 pour éviter le blocage du système
/*==========================================================*/
void RAZ_IT_RFM(void) {
	RFM_status(10);
	uchar buf[2];
	buf[0] = (char)(RFM12_READ >> 8);
	buf[1] = 0x00;
	write_SPI16(buf);
	return;
} 

////////////////////////////////////////////////////////////////////////
//                          RECEPTION                                  /
////////////////////////////////////////////////////////////////////////
/*==========================================================*/
/* MISE EN SERVICE RECEPTION */  /*  */
/*==========================================================*/
void RF12_recept_ES(void) {
  RF12_CMD (RFM12_RESET);						// et = 0,	er = 0 : arret émetteur et récepteur
	RF12_CMD (RFM12_CONF_OFF);				// el = ef = 0 : efface les IT ?
	RF12_CMD (RFM12_CONF_EF_ON);			// eF = 1
  RF12_CMD (RFM12_RECEP);
//	temps de démarrage de l'oscillateur : 5ms
#if defined(__linux__) || OSX	
	msleep(5);
#else		// EPS32
	usleep(1000*5);	// TODO ajuster tempo ??
#endif
	RAZ_IT_RFM();
  RF12_CMD (RFM12_FIFO_START);		// demarrage FIFO
}

// *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
//	gestion réception RFM12 en interruption
// *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
int reception_RFM12 (void *arg)
{
	_E_R_RFM12 *pE_R_RFM12 = arg;
	static	uint 	cpt;
	static	uint	nbrOctet;
	static	uint	CheckSum;
	int 	retour;
	uchar	recept;
	uchar *bufRX = pE_R_RFM12->bufRX;	
	retour = pE_R_RFM12->result;
	
	#if defined(__linux__) || OSX
	uchar	buf[2];
	int		status;
	status = lectFIFO(buf);
	if(status) {
		recept = buf[1];
	#else
		recept = pE_R_RFM12->octetRecu;
	#endif

INFO("reçu 0x%02X ", recept);
// LOGI("reçu 0x%02X ", recept);
		if(retour == RECU) {
INFOA("RECU non traité");
		}
		else if(((retour == VIDE) && (uint)recept != RESEAU)) {
INFOL("mauvais adressage result %i reseau 0x%02X", retour, recept);
			goto ret_VIDE;
		}
		else if(((retour == VIDE) && (uint)recept == RESEAU)) {
			retour = EN_COURS;
			nbrOctet = 0;
			CheckSum = 0;
			cpt = 0;
INFOL("RESEAU %i", recept);
		}
		else if((retour == EN_COURS) && (nbrOctet == 0) && ((uint)recept < SIZE_DATA +1) && ((uint)recept != 0)) {
			retour = EN_COURS;
			nbrOctet = (uint) recept;
			bufRX[0] = (uint) recept;							
			CheckSum = (uint) recept;
INFOL("nbrOctet %i", nbrOctet);
		}
		else if((cpt < nbrOctet) && (cpt < SIZE_DATA)) {
			retour = EN_COURS;
			cpt += 1;
			bufRX[cpt] = recept;
			CheckSum += recept;
INFO("reçu[%i] bufRX : 0x%02X ", cpt, bufRX[cpt]);
		}
		else if (cpt == nbrOctet) {
			if((CheckSum & 0xFF) == recept) {
				nbrOctet = 0;
				CheckSum = 0;
				cpt = 0;
				retour = RECU;
INFOL("RECU CheckSum OK");
			}
			else {
#if defined(__linux__) || OSX
				if(dbg > 3) {
					for(int i=0; i < SIZE_DATA; i++) {
						printf("0x%02X ", bufRX[i]);
					}
					printf("\n\t cpt %i erreur CheckSum calcul 0x%02X recu 0x%02X\n", cpt, CheckSum, recept);
				}
#endif
				goto ret_VIDE;
			}
		}
		else {
INFO("else ...");
			goto ret_VIDE;
		}
		
	#if defined(__linux__) || OSX
} // (RFM_status(1)
	#else
		
	pE_R_RFM12->result = retour;
// LOGI("0x%02X ret %i", recept, retour);
	return retour;
		
ret_VIDE:
	retour = VIDE;
	nbrOctet = 0;
	CheckSum = 0;
	cpt = 0;
	pE_R_RFM12->result = retour;
// LOGI("VIDE 0x%02X ret %i", recept, retour);
return retour;
}
#endif

/* ------------------------------------------------------------------------------- */
/* ------------------- FONCTIONS SPECFIQUE ESP32 --------------------------------- */
/* ------------------------------------------------------------------------------- */
#if defined(__linux__) || OSX
#else

	#define GPIO_IRQ_RFM12	pE_R_RFM12->irq_RFM		// entrée cablée sur sortie IRQ du RFM12

/*==========================================================*/
/* gestionnaire interruption réception RFM12 */ /*
	lorsque qu'une interruption se produit le signale via RFMIT_HandlerTask
 */
/* NOTE IMORTANTE INTERRUPTIONS */ /*
Il faut garder en tête que la fonction d’une interruption doit s’exécuter le plus rapidement possible pour ne pas perturber
le programme principal. Le code doit être le plus concis possible et il est déconseillé de dialoguer par SPI, I2C, UART 
depuis une interruption.
Avertissement
On ne peut pas utiliser la fonction delay() ni Serial.println() avec une interruption. On peut néanmoins afficher des
messages dans le moniteur série en remplaçant Serial.println() par ets_printf() qui est compatible avec les interruptions.
 */
/*==========================================================*/
void IRAM_ATTR interruption_RFM12(void *arg)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  vTaskNotifyGiveFromISR(RFMIT_HandlerTask, &xHigherPriorityTaskWoken );
  if( xHigherPriorityTaskWoken != pdFALSE ) {
    portYIELD_FROM_ISR();
  }
}
/*==========================================================*/
/* init interruption réception RFM12 */ /*
		- associe la pin E_R_RFM12->irq_RFM a une interruption
		- initialise la file d'attente pour transfert de données
		- surveille si une interruption est survenue, si oui, lit les données reçues et les insère dans la file d'attente
 */
/*==========================================================*/
void init_IT_RFM12(void *E_R_RFM12) {
	_E_R_RFM12	*pE_R_RFM12 = E_R_RFM12;
	uchar buf[2];
	
 //zero-initialize the config structure.
	gpio_config_t io_conf = {};
	//interrupt sur transition high vers low 
	io_conf.intr_type = GPIO_INTR_NEGEDGE;
	//bit mask pour gpio irq
	io_conf.pin_bit_mask = (1ULL<<GPIO_IRQ_RFM12);
	//set as input mode
	io_conf.mode = GPIO_MODE_INPUT;
	//enable pull-up mode
//   io_conf.pull_up_en = 1;
	gpio_config(&io_conf);

  UBaseType_t uxPriority;
  uxPriority = uxTaskPriorityGet( NULL );
	ESP_LOGW(TAG, "init_IT_RFM12 - core = %d (priorite %d)", xPortGetCoreID(), uxPriority);
	
	//install gpio isr service
	gpio_install_isr_service(ESP_INTR_FLAG_LEVEL3);
// 	gpio_install_isr_service(0);
	
// 	pE_R_RFM12->IT_queue = xQueueCreate(SIZE_FIFO * 3, sizeof( uint32_t ));

	//hook isr handler for specific gpio pin
	gpio_isr_handler_add(GPIO_IRQ_RFM12, interruption_RFM12, (void*) GPIO_IRQ_RFM12);

	// mise en service RFM12
	RF12_recept_ES();
	// NOTE test
	RAZ_IT_RFM();

	// boucle infini de surveillance des interruptions
	while(1) {
    if( ulTaskNotifyTake( pdTRUE, pdMS_TO_TICKS( 500 ))) {
			buf[0] = (char)(RFM12_READ >> 8);
			buf[1] = 0x00;
			write_SPI16(buf);
// 			xQueueSendToBack( pE_R_RFM12->IT_queue, (uchar *) &buf[1], ( TickType_t ) 5 );
			xQueueSendToBack( pE_R_RFM12->IT_queue, (uchar *) &buf[1], 5 / portTICK_PERIOD_MS ); // NOTE test modif
    }
	}	
}

/*==========================================================*/
/* Start interruption réception RFM12 */ /* 
		démarre le gestionnaire d'interruption de la pin GPIO_IRQ_RFM12
		Utilisé avec stopIT_RFM12 pour éviter les interruptions intempestives lors des émissions RFM12
 */
/*==========================================================*/
void startIT_RFM12(_E_R_RFM12 *pE_R_RFM12) {
	xQueueReset(pE_R_RFM12->IT_queue);
	// Lecture FIFO pour RAZ IT
	RF12_CMD (RFM12_CONF_OFF);				// el = ef = 0 : efface les IT ?
	RF12_CMD (RFM12_CONF_EF_ON);			// eF = 1
  RF12_CMD (RFM12_FIFO_START);			// demarrage FIFO  
	RAZ_IT_RFM();

    //hook isr handler for specific gpio pin again
// 	gpio_isr_handler_add(GPIO_IRQ_RFM12, interruption_RFM12, (void*) GPIO_IRQ_RFM12);
  	// Resume the suspended task ourselves.
  vTaskResume( RFMIT_HandlerTask );
}

/*==========================================================*/
/* Stop interruption réception RFM12 */ /* 
		arrête le gestionnaire d'interruption de la pin GPIO_IRQ_RFM12
		Utilisé avec startIT_RFM12 pour éviter les interruptions intempestives lors des émissions RFM12
 */
/*==========================================================*/
void stopIT_RFM12(_E_R_RFM12 *pE_R_RFM12) {
    //remove isr handler for gpio number.
// 	gpio_isr_handler_remove(GPIO_IRQ_RFM12);
		// Use the handle to suspend the created task.
  RF12_CMD (RFM12_FIFO_RESET);		// reset FIFO
  vTaskSuspend( RFMIT_HandlerTask );
	}

#endif	// EPS32

/* ------------------------------------------------------------------------------- */
/* ------------------- FONCTIONS SPECFIQUE OSX / RPI ----------------------------- */
//                         gestion liaison SPI
/* ------------------------------------------------------------------------------- */
#if defined(__linux__) || OSX
/*==========================================================*/
/* lecture FIFO */  /*  */
/*==========================================================*/
int lectFIFO(uchar *buf) {
	int ret = RFM_status(0);
	if(ret) {
		buf[0] = (char)(RFM12_READ >> 8);
		buf[1] = 0x00;
		write_SPI16(buf);
INFO("lectFIFO 0x%02X 0x%02X", buf[0], buf[1]);
	}
	return ret;
}
  
/*==========================================================*/
/* initialise SPI */ /*  */
/*==========================================================*/
void init_SPI(void)
{
	IM_SPI_START;
// 	bcm2835_set_debug(2);
	bcm2835_spi_set_speed_hz(2000000);
	IM_SPI_CS(SPI_CS0);
INFOA("spi RPI initialisée");
}

/*==========================================================*/
/* arrete SPI : restaure les registres du RaspeberryPi */ /*  */
/*==========================================================*/
void arret_SPI(void)
{
	IM_SPI_STOP;
	IM_RESTAURE;
INFOA("spi RPI arretée");
}

/*==========================================================*/
/* envoi 16 bits sur SPI et retourne 16 bits lus */  /*  */
/*==========================================================*/
int write_SPI16(uchar *buf) {
// INFOA("\t write data 0x%02X%02X", (uint)buf[0], (uint)buf[1]);
	bcm2835_spi_transfern((char*)buf, 2);
// INFOD("\t\t data read 0x%02X%02X", (uint)buf[0], (uint)buf[1]);
	#if OSX		// NOTE simu OSX
{	
  	dbg = 3; // TODO à supprimer ?
	if(buf[0] == 0 ) {
	}
	else if(buf[0] == 0xB8) {
		INFO("\t write data 0x%02X%02X", (uint)buf[0], (uint)buf[1]);
	}
	else if(buf[0] == 0xB0) {
		if(idx == -1){
			buf[1] = 1; // id reseau
			idx++;
		}
		else {
		buf[1] = dataRFM12[idx]; // NOTE simu OSX
		INFO("\t read data 0x%02X%02X idx%i", (uint)buf[0], (uint)dataRFM12[idx], idx);
		idx++;
		}
	}
	else {
		INFO("\t cmd 0x%02X%02X", (uint)buf[0], (uint)buf[1]);
	}
	buf[0] = isRGITFFIT;
  	dbg = 3; // TODO à supprimer ?
}
	#endif
  return 0;
}

#endif	// defined(__linux__) || OSX

