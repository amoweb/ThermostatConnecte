#ifndef RFM12_H
#define RFM12_H

#if defined(__linux__) || OSX
#else
	#include "freertos/FreeRTOS.h"
	#include "freertos/queue.h"
	#if CONFIG_TYPE_RFM12
		#define RFM12B
	#endif
#endif

// #define STRINGIFY(s) XSTRINGIFY(s)
// #define XSTRINGIFY(s) #s
// 
// #pragma message ("RFM12_FREQ " STRINGIFY(RFM12_FREQ))

/////////////////////////////////////////////////////////////
typedef unsigned int uint;
typedef unsigned char uchar;

//===========================================================================
//                      Configuration émetteur                              =
 /*  Notes:
commonly used parameters
Module(modulation)     output  Rate/spread   frequency bandwidth Form
Center frequency       Power   ing factor    deviation
  434MHz              >8dbm   1.2Kbps       30KHz   134KHz  TX/RX 
  868MHz              5dbm    >2.4Kbps    
  915MHz              2dbm    4.8Kbps

      PRECAUTIONS IMPORTANTES !
1/ accorder la largeur de bande du recepteur (bandwidth RFM12_CMD_RX_CTL) avec la vitesse de 
transmission suivant tableau
2/ accorder la deviation de frequence de l'emetteur (modulation RFM12_CMD_TX_CTL) avec la 
vitesse de transmission suivant tableau
3/ le module RFM12 continue a générer des interruptions lorsque qu'il a été synchronisé même s'il n'y
	a plus d'émission.
*/
//===========================================================================
   // 433, 868 ou 915 bande de frequence du module
#if defined(__linux__) || OSX
	#define FREQ_0	433
// #define FREQ_0	868
// #define FREQ_0	915
	#if (FREQ_0 == 433)
	#define S_RFM_2		"SE_12"		// 433 Mhz mesure de température avec DS18B20
	#elif (FREQ_0 == 868)
	#define S_RFM_2		"SE_02"		// 868 Mhz mesure de température avec DS18B20
	#endif
#else
	#if defined(CONFIG_F_RFM_433)
		#define FREQ_0	433
		#define S_RFM_2		"SE_12"		// 433 Mhz mesure de température avec DS18B20
	#elif defined(CONFIG_F_RFM_868)
		#define FREQ_0	868
		#define S_RFM_2		"SE_02"		// 868 Mhz mesure de température avec DS18B20
	#else
		#define FREQ_0	868
	#endif
#endif


//   Band    Minimum Frequency   Maximum Frequency   PLL Frequency Step
// 433 MHz     430.2400 MHz        439.7575 MHz        2.5kHz
// 868 MHz     860.4800 MHz        879.5150 MHZ        5.0kHz
// 915 MHz     900.7200 MHz        929.2725 MHz        7.5kHz
#if (FREQ_0 == 433)
#define FREQ_CHOISIE	434150  // frequence choisie en kHZ
// #warning "frequence cenrale 433 MHz, frequence choisie 434150 kHz"
#elif (FREQ_0 == 868)
#define FREQ_CHOISIE	866500  // frequence choisie en kHZ
// #warning "frequence cenrale 868 MHz, frequence choisie 866500 kHz"
#elif (FREQ_0 == 915)
#define FREQ_CHOISIE	915300  // frequence choisie en kHZ
// #warning "frequence cenrale 915 MHz, frequence choisie 915300 kHz"
#else
//   error "Frequence centrale[#v(FREQ_0)] non conforme: 433, 868 ou 915"
#endif

//===========================================================================

// ; format messages RFM12 (les octets 1 à SIZE_FIFO - 2 servent au calcul du checksum)
// ;			octet						| 			Fonction
// ;       _							| id réseau (RESEAU)
// ;       0							| nombre d'octets
// ;       1							| M (Maitre) ou S(Slave)
// ;       2							| E (Emetteur) ou _
// ;       3							| R (Recepteur) ou _
// ;       4							| 0-9 numéro du groupe
// ;       5							| 0-9 numéro du module
// ;       6							| état renvoyé par le module
// ;	7 à SIZE_FIFO - 2 	| data
// ;	SIZE_FIFO - 1 			|	checksum

#define RESEAU			1		// identifiant du réseau
#define S_RFM_1		"SER11"
#define S_RFM1X		"SER1X"		// mesage aux module du groupe 1

#define	RFM_ID			1					// position de l'id du module
#define	RFM_ETAT		6					// position de l'octet d'état renvoyé par le module
#define RFM_DATA		7					// position du premier octet de data envoyé par le module
//   => taille utile de données = SIZE_FIFO - 2 (-2 octets : nbr octets et checksum)
#define SIZE_DATA		14
// taille du buffer FIFO de RFM12 = SIZE_DATA + 2 (nbr octets et checksum)
#define SIZE_FIFO		SIZE_DATA + 2

// défauts renvoyés par l'émetteur RFM12
#define DEFAUT_BAT    0x01
#define ERR_CKS       0x02
#define ERR_18B20     0x04
#define ETAT_LED			0x01

typedef enum {NO_RUN, RUN, STOP, EMISSION, RECEPTION} ETAT_RFM;
typedef enum {VIDE, EN_COURS, RECU} ETAT_RECEPT;

typedef struct _E_R_RFM12 _E_R_RFM12;
struct _E_R_RFM12{
	pthread_mutex_t mutex;		// verrou
	uchar bufRX[SIZE_FIFO];		// buffer de reception des données
#if defined(__linux__) || OSX
#else
	int irq_RFM;							// pin entrée d'interruption (GPIO_INPUT_IRQ)
	QueueHandle_t	IT_queue;		// file d'attente pour transfert des données reçues 
	uchar	octetRecu;					// nbre d'octets disponible dans IT_queue
#endif
	int 	etat;								// optionnel pour debug indique l'état de la connection (enum ETAT_RFM)
	int		result;							// indique si l'état de la reception (enum ETAT_RECEPT)
};

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*  fonctions globales */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
#if defined(__linux__) || OSX
void arret_SPI(void);
#endif
_E_R_RFM12 *Init_RF12(void);

void envoiMessRFM12(char *data);
int reception_RFM12 (void *arg);
// void RFM12_reset_FIFO(void);
void startIT_RFM12(_E_R_RFM12 *pE_R_RFM12);
void stopIT_RFM12(_E_R_RFM12 *pE_R_RFM12);

//===========================================================================
//                             DEFINITIONS                                  =
//===========================================================================

// POR = power on reset values
//////////////////////////////////////////////////////////
// NOTE RFM12_CMD_CONFIG
#define RFM12_CONFIG_BASE	0x8000    // Configuration (POR 8008h)
// BIT  15 14 13 12 11 10 9 8  7  6  5  4  3  2  1  0
//      1   0  0  0  0  0 0 0  EL EF B1 B0 X3 X2 X1 X0 
//   X3 X2 X1 X0 .loadcap - Crystal load Capacitor
// Can be used to adjust/Calibrate the XTAL Frequency startup time
#define RFM12_CAP_085   0x00    //  8.5pF
#define RFM12_CAP_090   0x01    //  9.0pF
#define RFM12_CAP_095   0x02    //  9.5pF
#define RFM12_CAP_100   0x03    // 10.0pF
#define RFM12_CAP_105   0x04    // 10.5pF
#define RFM12_CAP_110   0x05    // 11.0pF
#define RFM12_CAP_115   0x06    // 11.5pF
#define RFM12_CAP_120   0x07    // 12.0pF
#define RFM12_CAP_125   0x08    // 12.5pF - POR
#define RFM12_CAP_130   0x09    // 13.0pF
#define RFM12_CAP_135   0x0A    // 13.5pF
#define RFM12_CAP_140   0x0B    // 14.0pF
#define RFM12_CAP_145   0x0C    // 14.5pF
#define RFM12_CAP_150   0x0D    // 15.0pF
#define RFM12_CAP_155   0x0E    // 15.5pF
#define RFM12_CAP_160   0x0F    // 16.0pF 
// B1 B0 .band - Band of module  
// The band is module specific and needs to be matched to the physical RFM12 module you have.
// If in doubt check the back of the RFM module.
// i.e. you can't set 915Mhz if your module is physically a 433Mhz
#define RFM12_BAND_NONE   0x00    // POR
#define RFM12_BAND_433    0x10    // 433MHz
#define RFM12_BAND_868    0x20    // 868MHz
#define RFM12_BAND_915    0x30    // 915MHz

#if (FREQ_0 == 433)
#define RFM12_BAND        RFM12_BAND_433    // 433MHz
#elif (FREQ_0 == 868)
#define RFM12_BAND        RFM12_BAND_868    // 868MHz
#elif (FREQ_0 == 915)
#define RFM12_BAND        RFM12_BAND_915    // 915MHz
#else
#define RFM12_BAND        RFM12_BAND_NONE    // POR
#endif

#define RFM12_8000_EF     0x40    // ef : Enables FIFO Mode
#define RFM12_8000_EL     0x80    // el : Enable internal data register

// #define RFM12_CONF_REG_ON		RFM12_CONFIG_BASE | RFM12_CAP_125 | RFM12_BAND | RFM12_8000_EF | RFM12_8000_EL
#define RFM12_CONF_REG_ON		RFM12_CONFIG_BASE | RFM12_CAP_125 | RFM12_BAND | RFM12_8000_EL
// #define RFM12_CONF_REG_OFF	RFM12_CONFIG_BASE | RFM12_CAP_125 | RFM12_BAND | RFM12_8000_EF
#define RFM12_CONF_EF_ON		RFM12_CONFIG_BASE | RFM12_CAP_125 | RFM12_BAND | RFM12_8000_EF
#define RFM12_CONF_OFF			RFM12_CONFIG_BASE | RFM12_CAP_125 | RFM12_BAND
  
/////////////////////////////////////////////////////////////
// NOTE RFM12_CMD_POWERMGMT
#define RFM12_POWERMGMT_BASE	0x8200    // Power management (POR 8208h)
// BIT  15 14 13 12 11 10 9 8  7  6   5  4  3  2  1  0
//      1   0  0  0  0  0 1 0  ER EBB ET ES EX EB EW DC 
// DC Disable clock
#define RFM12_CLK_OFF     0x01    // dc :	CLK pin off
#define RFM12_CLK_ON      0x00    // 			CLK pin on - POR
// EW Enable wakeup
#define RFM12_WAKEUP_OFF  0x00    // ew : wakeup timer disabled - POR
#define RFM12_WAKEUP_ON   0x02    // 			Wakeup timer enabled
// EB Enable low battery detector
#define RFM12_LOWBAT_OFF  0x00    // eb :	Low Battery detector off - POR
#define RFM12_LOWBAT_ON   0x04    // 			Low Battery detector on
// EX Enable xtal Oscillator
#define RFM12_XTAL_OFF    0x00    // ex :	Crystal Oscillacitor Off
#define RFM12_XTAL_ON     0x08    // 			Crystal Oscillacitor On - POR
// ES enable synthesizer
#define RFM12_SYNTH_OFF   0x00    // es :	Synthesizer Off - POR
#define RFM12_SYNTH_ON    0x10    // 			Synthesizer On
// ET enable transmitter
#define RFM12_TX_OFF      0x00    // et : Transmitter Off - POR
#define RFM12_TX_ON       0x20    // 			Transmitter On
// EBB enable base band block
#define RFM12_EBB_OFF     0x00    // ebb : Base Band Block Off - POR
#define RFM12_EBB_ON      0x40    // 			 Base Band Block On
// ER enable receiver
#define RFM12_RX_OFF      0x00    // er :	Receiver Off - POR
#define RFM12_RX_ON       0x80    // er :	Receiver On

#define RFM12_RESET 	RFM12_POWERMGMT_BASE | RFM12_CLK_OFF | RFM12_WAKEUP_OFF | RFM12_LOWBAT_ON \
											| RFM12_XTAL_ON | RFM12_SYNTH_ON | RFM12_TX_OFF | RFM12_EBB_ON | RFM12_RX_OFF

#define RFM12_EMIS 		RFM12_POWERMGMT_BASE | RFM12_CLK_OFF | RFM12_WAKEUP_OFF | RFM12_LOWBAT_ON \
											| RFM12_XTAL_ON | RFM12_SYNTH_ON | RFM12_TX_ON | RFM12_EBB_ON | RFM12_RX_OFF

#define RFM12_RECEP 	RFM12_POWERMGMT_BASE | RFM12_CLK_OFF | RFM12_WAKEUP_OFF | RFM12_LOWBAT_ON \
											| RFM12_XTAL_ON | RFM12_SYNTH_ON | RFM12_TX_OFF | RFM12_EBB_ON | RFM12_RX_ON
  
/////////////////////////////////////////////////////////////
// NOTE RFM12_CMD_FREQ
#define RFM12_FREQ_BASE	0xA000    // Frequency control (POR A680h)
// BIT  15 14 13 12 11  10   9  8  7  6  5  4  3  2  1  0
//      1   0  1  0 F11 F10 F9 F8 F7 F6 F5 F4 F3 F2 F1 F0
#define RFM12_A000_FREQ_OFF   0x640 // Frequence typique (433, 868 ou 915) 
#define RFM12_A000_FREQ_MASK    0x7FF // Operating frequency bit mask
// => fréquence réelle est fonction de la bande choisie
// The 12-bit parameter F (bits f11 to f0) should be in the range of 96 and 3903. When F value sent is out of range,
// the previous value is kept. The synthesizer center frequency f0 can be calculated as: 
// f0 = 10 · C1 · (C2 + F/4000) [MHz]
// F = (f0 - 900)/0.0075 (915MHz)
// F = (f0 - 860)/0.005  (866MHz)
// F = (f0 - 430)/0.0025 (433MHz)
// Where:
// f0  = Centre Frequency in MHz
// F   = 11 bit register value maps to f11:f0 of the Frequency CMD Register (A000)
// Band (MHz)    C1  C2
// 433           1   43
// 868           2   43
// 915           3   30
#if (FREQ_0 == 433)
// Calculate the RFM12 register value for a given Frequency at 433MHz
#define RFM12_FREQ_433    ((FREQ_CHOISIE-430000)*10/25)
#define RFM12_FREQ	RFM12_FREQ_BASE | RFM12_FREQ_433 // (0xA67C) 434,15 MHz
#endif
#if (FREQ_0 == 868)
// Calculate the RFM12 register value for a given Frequency at 868 MHz
#define RFM12_FREQ_868    ((FREQ_CHOISIE-860000)/5)
#define RFM12_FREQ	RFM12_FREQ_BASE  | RFM12_FREQ_868 // (0xA67C) 868,3 MHz
#endif
#if (FREQ_0 == 915)
// Calculate the RFM12 register value for a given Frequency at 915MHz
#define RFM12_FREQ_915    ((FREQ_CHOISIE-900000)*10/75)
#define RFM12_FREQ	RFM12_FREQ_BASE | RFM12_FREQ_915
#endif


/////////////////////////////////////////////////////////////
// NOTE RFM12_CMD_RATE
#define RFM12_RATE_BASE	0xC600  // Data rate (POR C623h)
// BIT  15 14 13 12 11 10 9 8  7  6   5  4  3  2  1  0
//      1   1  0  0  0  1 1 0  CS R6 R5 R4 R3 R2 R1 R0 
// .bitrate - RF baudrate 
// 7 bit value
// Value of register is calculated as:
// bitRate = 344828 / (1 + cs*7) / (BR- 1) 
// Where:
// bitrate = 7 bit value
// cs = 0 or 1
// BR = Required bitrate in kbps  i.e. 9.600 = 9600bps
// So any value from 600 - 115200 can be calculated
// below is some standard ones and their CS value
#define RFM12_BITRATE_115200  0x02  // 115.2 kbps  CS=0
#define RFM12_BITRATE_57600   0x05  // 57600 bps   CS=0
#define RFM12_BITRATE_19200   0x11  // 19200 bps   CS=0
#define RFM12_BITRATE_9600    0x23  // 9600  bps   CS=0
#define RFM12_BITRATE_4800    0x47  // 4800  bps   CS=0
#define RFM12_BITRATE_2400    0x8F  // 2400  bps   CS=1
#define RFM12_BITRATE_1200    0xA3  // 1200  bps   CS=1

#define RFM12_RATE RFM12_RATE_BASE | RFM12_BITRATE_4800
// #define RFM12_RATE RFM12_RATE_BASE | RFM12_BITRATE_9600
 
/////////////////////////////////////////////////////////////
#define RFM12_RX_CTL_BASE	0x9000  // Receiver control (POR 9080h)
// NOTE RFM12_CMD_RX_CTL
// BIT  15 14 13 12 11 10  9   8  7  6  5  4  3  2  1  0
//      1   0  0  1  0 P16 D1 D0 I2 I1 I0 G1 G0 R2 R1 R0
// R2 R1 R0 .rssi - Relative Signal Strength Indicator
// As the RSSI Threshold relies on the LNA gain the true RSSI can be calculated by:
// RDDIth  = RSSIsetth + Glna
//   Where:
// RSSIth  = RSSI Threshold (Actual)
// RSSIsetth = RSSI Set Threshold (from below)
// Glna  = Gain LNA   (from below)
#define RFM12_RSSI_103  0x00    // -103dB - POR
#define RFM12_RSSI_97   0x01    // -97dB 
#define RFM12_RSSI_91   0x02    // -91dB 
#define RFM12_RSSI_85   0x03    // -85dB 
#define RFM12_RSSI_79   0x04    // -79dB 
#define RFM12_RSSI_73   0x05    // -73dB 
// G1 G0 .lna - Rx Low noise amplifier Relative to Max 
#define RFM12_LNA_0     0x00    // -0dB - POR
#define RFM12_LNA_6     0x08    // -6dB 
#define RFM12_LNA_14    0x10    // -14dB 
#define RFM12_LNA_20    0x18    // -20dB 
//  I2 I1 I0 .bandwidth
//  Table of optimal bandwidth and transmitter deviation settings for given data rates 
//  (the data sheet was a bit vague in this area and did not specify frequencies etc so I don't know 
//   how valid they are at different frequency bands but they could be used as a starting point)
// data rate   bandwidth deviation
// 1200bps   67kHz   45kHz
// 2400      67      45
// 4800      67      45
// 9600      67      45
// 19200     67      45
// 38400     134     90
// 57600     134     90
// 115200    200     120
#define RFM12_BW_400    0x20    // 400kHz
#define RFM12_BW_340    0x40    // 340kHz
#define RFM12_BW_270    0x60    // 270kHz
#define RFM12_BW_200    0x80    // 200kHz - POR 
#define RFM12_BW_134    0xA0    // 134kHz
#define RFM12_BW_67     0xC0    // 67kHz
// D1 D0 .VDI Response time
#define RFM12_VDI_FAST    0x00    // VDI FAST
#define RFM12_VDI_MEDIUM  0x40    // VDI Medium
#define RFM12_VDI_SLOW    0x80    // VDI Slow
#define RFM12_VDI_ALWAYS  0xA0    // VDI Always on
// Pin 16 function - VDI output or interrupt input
#define RFM12_P16_OUT     0x400   // VDI output

// !!! LNA BW
// #define RFM12_RX_CTL RFM12_RX_CTL_BASE | RFM12_RSSI_103 | RFM12_LNA_14 | RFM12_BW_134 | RFM12_VDI_FAST | RFM12_P16_OUT
#define RFM12_RX_CTL RFM12_RX_CTL_BASE | RFM12_RSSI_103 | RFM12_LNA_0 | RFM12_BW_134 | RFM12_VDI_FAST | RFM12_P16_OUT
  
/////////////////////////////////////////////////////////////
// NOTE RFM12_CMD_FILTER
#define RFM12_FILTER_CTL_BASE	0xC228    // Filter control (C22Ch POR)
// BIT  15 14 13 12 11 10 9 8  7  6  5 4 3 2  1  0
//      1   1  0  0  0  0 1 0  AL ML 1 S 1 F2 F1 F0
// F2 F1 F0 .DQD threshold
#define RFM12_C200_S      0x30    // Filter Type 0=digital 1=analogue
#define RFM12_C200_ML     0x60    // Clock recovery mode lock  mode 0=slow 1=fast
#define RFM12_C200_AL     0xA0    // Clock recovery auto lock  0=manuel 1=auto
#define RFM12_C200_F_OFF  0x00    // DQD threshold
#define RFM12_C200_F_MASK 0x07    // DQD threshold mask
// !!! DQD
#define RFM12_FILTER_CTL	RFM12_FILTER_CTL_BASE | RFM12_C200_AL | RFM12_C200_ML | 0x04
  
/////////////////////////////////////////////////////////////
// NOTE RFM12_CMD_FIFORST_CTL
//Note: To restart the synchron pattern recognition, bit 1 should be cleared and set.
#define RFM12_FIFO_CTL_BASE	0xCA00    // FIFO and reset control (POR CA80h)
// BIT  15 14 13 12 11 10 9 8  7  6   5  4 3   2  1  0
//      1   1  0  0  1  0 1 0  F3 F2 F1 F0 SP AL FF DR
#define RFM12_CA00_DR   0x01    // Disable Reset Mode
#define RFM12_CA00_FF   0x02    // FIFO fill enabled after reception of Sync pattern
#define RFM12_CA00_AL   0x04    // Fifo Fill start condition 0=sync, 1=Always fill
#define RFM12_CA00_SP   0x08    // Length of Sync Patten 0=2 bytes (0x2DD4), 1=1byte(0xD4)
// F3 F2 F1 F0 .FIFO Interrupt Level
#define RFM12_CA00_F_OFF		0x00    // FIFO Interrupt Level, generates IT when x number of bits received
#define RFM12_CA00_F_MASK		0xF0    // FIFO Interrupt Level mask
#define RFM12_CA00_F_16bit	0xF0    // FIFO Interrupt après réception de 16 bits
#define RFM12_CA00_F_8bit 	0x80    // FIFO Interrupt après réception de 8 bits
// NOTE FIFO
#define RFM12_FIFO_RESET	RFM12_FIFO_CTL_BASE | RFM12_CA00_DR | RFM12_CA00_F_8bit
#define RFM12_FIFO_START	RFM12_FIFO_CTL_BASE | RFM12_CA00_DR | RFM12_CA00_FF | RFM12_CA00_F_8bit
// #define RFM12_FIFO_START	RFM12_FIFO_CTL_BASE | RFM12_CA00_DR | RFM12_CA00_FF | RFM12_CA00_F_16bit
  
#ifdef RFM12B
/////////////////////////////////////////////////////////////
// NOTE RFM12_CMD_SYNC
#define RFM12_SYNC_BASE	0xCE00  // Sync pattern (POR CED4h)
// BIT  15 14 13 12 11 10 9 8  7  6  5   4  3  2  1  0
//      1   1  0  0  1  1 1 0  B7 B6 B5 B4 B3 B2 B1 B0
#define RFM12_CE00_B_OFF  0x00    // Sync Patten - User programmable
#define RFM12_CE00_B_MASK 0xFF    // Sync Patten mask

#define RFM12_SYNC	RFM12_SYNC_BASE | 0xD4
#endif

/////////////////////////////////////////////////////////////
#define RFM12_FIFO_READ	0xB000  // FIFO read command
#define RFM12_READ	RFM12_FIFO_READ 
  
/////////////////////////////////////////////////////////////
// NOTE RFM12_CMD_AFC_CTL
#define RFM12_AFC_CTL_BASE	0xC400  // AFC read command (POR C4F7h)
// BIT  15 14 13 12 11 10 9 8  7  6    5   4  3  2  1  0
//      1   1  0  0  0  1 0 0  A1 A0 RL1 RL0 ST FI OE EN
#define RFM12_C400_EN   0x01    // AFC Enable
#define RFM12_C400_OE   0x02    // AFC Enable frequency offset register
#define RFM12_C400_FI   0x04    // AFC Enable High Accurracy (fine) mode
#define RFM12_C400_ST   0x08    // AFC Strobe Edge When high AFC latest is stored in offset register
//   RL1 RL0 .afc_range Limit
// Limits the value of the frequency offset register to the next values fRes:
//   315,433Mhz bands:   2.5kHz
//   868MHz band:        5.0kHz
//   915MHz band:        7.5kHz
#define RFM12_AFC_RNG_NONE  0x00    // No Restriction
#define RFM12_AFC_RNG_1516  0x10    // +15/-16
#define RFM12_AFC_RNG_0708  0x20    // +7/-8
#define RFM12_AFC_RNG_0304  0x30    // +3/-4  - POR
// A1 A0 .afc   Automatic Frequency Control  
// RFM12_AFC_PWRUP Recommended for Max distance
// RFM12_AFC_VDI   Recommended for receiving from Multiple Transmitters (Point To Multipoint) 
// RFM12_AFC_nVDI  Recommended for receiving from Single Transmitter (PtP)
#define RFM12_AFC_MCU     0x00    // Controlled by MCU
#define RFM12_AFC_PWRUP   0x40    // Runs Once at powerup
#define RFM12_AFC_VDI     0x80    // Keeps Offset when VDI High
#define RFM12_AFC_nVDI    0xC0    // Keeps Offset independent of VDI - POR
// !!! AFC
#define RFM12_AFC_CTL	RFM12_AFC_CTL_BASE | RFM12_C400_EN | RFM12_C400_OE | RFM12_AFC_RNG_0708 | RFM12_AFC_VDI
// #define RFM12_AFC_CTL	RFM12_AFC_CTL_BASE
// #define RFM12_AFC_CTL	RFM12_AFC_CTL_BASE | RFM12_C400_EN | RFM12_C400_OE | RFM12_C400_FI | RFM12_AFC_RNG_0708 | RFM12_AFC_nVDI

/////////////////////////////////////////////////////////////
// NOTE RFM12_CMD_TX_CTL
#define RFM12_TX_CTL_BASE	0x9800  // Tx config control command (POR 9800h)
// BIT  15 14 13 12 11 10 9 8  7  6  5  4  3 2  1  0
//      1   0  0  1  1  0 0 MP M3 M2 M1 M0 0 P2 P1 P0
// .power - Tx Output Power
// The output power is relative to the maximium power available, which depnds on the actual 
// antenna impedenance.
#ifdef RFM12B
	#define RFM12_POWER_0     0x00    //  0dB - POR
	#define RFM12_POWER_3     0x01    // -3dB
	#define RFM12_POWER_6     0x02    // -6dB
	#define RFM12_POWER_9     0x03    // -9dB
	#define RFM12_POWER_12    0x04    // -12dB
	#define RFM12_POWER_15    0x05    // -15dB
	#define RFM12_POWER_18    0x06    // -18dB
	#define RFM12_POWER_21    0x07    // -21dB
#else
	#define RFM12_POWER_0     0x00    //  0dB - POR
	#define RFM12_POWER_2_5		0x01    // -3dB
	#define RFM12_POWER_5     0x02    // -6dB
	#define RFM12_POWER_7_5   0x03    // -9dB
	#define RFM12_POWER_10    0x04    // -12dB
	#define RFM12_POWER_12_5  0x05    // -15dB
	#define RFM12_POWER_15    0x06    // -18dB
	#define RFM12_POWER_17_5  0x07    // -21dB
#endif
// MP M3 M2 M1 M0 .modulation  - Frequency Deviation
// Transmit FSK Modulation Control
// modulation (kHz) = f0 + (-1)^sign * (M +1) * (15kHZ)
// Where:
//   f0   = channel centre frequency
//   M    = 4 bit binary bumber <m3:m0>
//   SIGN = (mp) XOR (Data bit)
// 
//     Pout
//       ^
//       |       ^       |       ^
//       |       |       |       |
//       |       |<dfFSK>|<dfFSK>|
//       |       |       |       |
//       +-------+-------+-------+--------> fout
//                       f0
//               ^               ^
//                       |       +------- mp=0 and FSK=1 or mp=1 and FSK=0
//               |
//               +------------------------mp=0 and FSK=0 or mp=1 and FSK=1         
//*/
#define RFM12_MODULATION_15     0x00    // 15 kHz - POR
#define RFM12_MODULATION_30     0x10    // 30 kHz  => 4800 bds
#define RFM12_MODULATION_45     0x20    // 45 kHz
#define RFM12_MODULATION_60     0x30    // 60 kHz
#define RFM12_MODULATION_75     0x40    // 75 kHz
#define RFM12_MODULATION_90     0x50    // 90 kHz
#define RFM12_MODULATION_105    0x60    // 105 kHz
#define RFM12_MODULATION_120    0x70    // 120 kHz
#define RFM12_MODULATION_135    0x80    // 135 kHz
#define RFM12_MODULATION_150    0x90    // 150 kHz
#define RFM12_MODULATION_165    0xA0    // 165 kHz
#define RFM12_MODULATION_180    0xB0    // 180 kHz
#define RFM12_MODULATION_195    0xC0    // 195 kHz
#define RFM12_MODULATION_210    0xD0    // 210 kHz
#define RFM12_MODULATION_225    0xE0    // 225 kHz
#define RFM12_MODULATION_240    0xF0    // 240 kHz

//  !!! modulation
#define RFM12_TX_CTL RFM12_TX_CTL_BASE |	RFM12_POWER_0 | RFM12_MODULATION_90
// #define RFM12_TX_CTL RFM12_TX_CTL_BASE |	RFM12_POWER_0 | RFM12_MODULATION_60

#ifdef	RFM12B
/////////////////////////////////////////////////////////////
// RFM12_CMD_PLL_CTL
#define RFM12_PLL_CTL_BASE	0xCC02  // PLL control command (POR CC77h)
// BIT  15 14 13 12 11 10 9 8  7  6   5   4   3   2  1  0
//      1   1  0  0  1  1 0 0 OB1 OB0 1   LPX DDY DDIT 1 BW0
#define RFM12_CC00_BW_ON  0x01    // PLL bandwidth max bit rate[kbps] 0=86.2 1=256)
#define RFM12_CC00_DDIT   0x04    // Disables Dithering in PLL Loop
#define RFM12_CC00_DDY    0x08    // Phase Detector Delay
#define RFM12_CC00_LPX    0x10    // Low Power mode of Crystal
// OB1 OB0 .Output clock buffer    // ob1 ob0 Selected μC CLK frequency
#define RFM12_OB_5        0x60    // 1 1 : 5 or 10 MHz (recommended)
#define RFM12_OB_3        0x20    // 0 1 : 3.3MHz
#define RFM12_OB_2        0x40    // 1 X : 2.5 MHz or less

#define RFM12_PLL_CTL	RFM12_PLL_CTL_BASE | RFM12_CC00_BW_ON | RFM12_CC00_DDIT | RFM12_OB_5
#endif

/////////////////////////////////////////////////////////////
// RFM12_CMD_TX_WRITE
#define RFM12_TX_WRITE_BASE	0xB800  // Tx write buffer command
// BIT  15 14 13 12 11 10 9 8  7  6  5  4  3  2  1  0
//      1   1  0  0  1  0 0 0  T7 T6 T5 T4 T3 T2 T1 T0
#define RFM12_B800_T      0x00    // Transmit Register Write
#define RFM12_B800_T_MASK 0xFF    // Transmit Register Write Mask

#define RFM12_WRITE	RFM12_TX_WRITE_BASE
  
/////////////////////////////////////////////////////////////
// RFM12_CMD_WAKEUP
#define RFM12_WAKEUP_BASE	0xE100  // Wakeup timer command (POR E196h)
// BIT  15 14 13 12 11 10 9  8  7  6  5  4  3  2  1  0
//      1   1  1 R4 R3 R2 R1 R0 M7 M6 M5 M4 M3 M2 M1 M0
#define RFM12_E100_M_OFF    0x000 // Wakeup time period
#define RFM12_E100_M_MASK   0x0FF // Wakeup time period mask
#define RFM12_E100_R_OFF    0x000 // Wakeup time period use values 0-29
#define RFM12_E100_R_MASK   0x1F0 // Wakeup time period mask
// .wakeup_period
// Twake-up = M * 2^R[ms] pour RFM 12
// Twake-up = 1.03·M·2^R +0.5[ms] pour RFM12B
// Where:
// T = Time in ms of wakeup timer
// M = 8 bit value - maps to bits 7 - 0 of Wake up timer command Register
// R = 5 bit value - maps to bits 12- 8 of Wake up timer command Register
// NOTE:  
//   1) For future compatibility  0 <= R <= 29
//   2) For continual Operation the EW bit should be cleared and 
//      set at end of every cycle.

#define RFM12_WAKEUP	RFM12_WAKEUP_BASE | RFM12_E100_M_OFF | RFM12_E100_R_OFF
  
/////////////////////////////////////////////////////////////
// RFM12_CMD_DUTYCYCLE
#define RFM12_DUTYCYCLE_BASE	0xC800  // Duty cycle command (POR C80Eh)
// BIT  15 14 13 12 11 10 9 8  7  6  5  4  3  2  1  0
//      1   1  0  0  1  0 0 0 D6 D5 D4 D3 D2 D1 D0 EN
#define RFM12_C800_EN   0x01    // Enable low duty-cycle mode
#define RFM12_C800_D_OFF  0x00    // Duty-cycle OFF
// Duty-Cycle= (D * 2 +1) / M *100%
// D6 D5 D4 D3 D2 D1 D0 .Duty-cycle mask
#define RFM12_C800_D_MASK 0xFE    // Duty-cycle mask

#define RFM12_DUTYCYCLE	RFM12_DUTYCYCLE_BASE | RFM12_C800_D_OFF | 0x00

/////////////////////////////////////////////////////////////
// RFM12_CMD_LOWBAT_CLK
#define RFM12_LOWBAT_CLK_BASE	0xC000  // Low battery detector and CLK pin clock divider (POR C000h)
// BIT  15 14 13 12 11 10 9 8  7  6  5  4  3  2  1  0
//      1   1  0  0  0  0 0 0  D2 D1 D0 0 V3 V2 V1 V0
// V3 V2 V1 V0 .lowbat_threshold 
// The EB bit (bit 2) needs to be set in the Power Management Register (C8200) to enable the Low Battery Monitor
#define RFM_LOWBAT_2_2    0x00    // 2.2V  - POR
#define RFM_LOWBAT_2_3    0x01    // 2.3V
#define RFM_LOWBAT_2_4    0x02    // 2.4V
#define RFM_LOWBAT_2_5    0x03    // 2.5V
#define RFM_LOWBAT_2_6    0x04    // 2.6V
#define RFM_LOWBAT_2_7    0x05    // 2.7V
#define RFM_LOWBAT_2_8    0x06    // 2.8V
#define RFM_LOWBAT_2_9    0x07    // 2.9V
#define RFM_LOWBAT_3_0    0x08    // 3.0V
#define RFM_LOWBAT_3_1    0x09    // 3.1V
#define RFM_LOWBAT_3_2    0x0A    // 3.2V
#define RFM_LOWBAT_3_3    0x0B    // 3.3V
#define RFM_LOWBAT_3_4    0x0C    // 3.4V
#define RFM_LOWBAT_3_5    0x0D    // 3.5V
#define RFM_LOWBAT_3_6    0x0E    // 3.6V
#define RFM_LOWBAT_3_7    0x0F    // 3.7V
//   D3 D2 D1 D0 .clk_speed  - speed of external CLK Pin
//   Divider for the internal RFM12 Oscillator on the External CLK ping can be used as clock source source 
//   for a uController.
//   Also needs the DC bit(bit 0) cleared in the Power management Register to enable CLK Pin.
#define RFM12_XTAL_100    0x00    // 1.00MHz - POR
#define RFM12_XTAL_125    0x20    // 1.25MHz
#define RFM12_XTAL_166    0x40    // 1.66MHZ
#define RFM12_XTAL_200    0x60    // 2.00MHZ
#define RFM12_XTAL_250    0x80    // 2.50MHZ
#define RFM12_XTAL_333    0xA0    // 3.33MHZ
#define RFM12_XTAL_500    0xC0    // 5.00MHZ
#define RFM12_XTAL_1000   0xE0    // 10.00MHZ

#define RFM12_LOWBAT_CLK	RFM12_LOWBAT_CLK_BASE | RFM_LOWBAT_2_3 | RFM12_XTAL_100
  
/////////////////////////////////////////////////////////////
// RFM12_STATUS_READ
#define RFM12_STATUS_READ_BASE set   0x0000  // Read status Register
// BIT  15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
//      0  X  X  X  X  X  X X X X X X X X X X    
//      This is result of status read
#define RFM12_RGIT_FFIT	 0x8000  // TX ready for next byte or FIFO received data Status
                  							 // depends on mode of transmitter          
#define RFM12_POR        0x4000  // Power on Reset Status
#define RFM12_RGUR_FFOV	 0x2000  // TX Register underun or RX FIFO Overflow Status
                  							 // depends on mode of transmitter
#define RFM12_WKUP       0x1000  // Wakeup Timer overflow Status
#define RFM12_EXT        0x0800  // Interrup on external source Status
#define RFM12_LBD        0x0400  // Low battery detect Status
#define RFM12_FFEM       0x0200  // FIFO Empty Status
#define RFM12_ATS        0x0100  // Antenna Tuning Signal Detect Status
#define RFM12_RSSI       0x0080  // Received Signal Strength Indicator Status
#define RFM12_DQD        0x0040  // Data Quality Dedector Status
#define RFM12_CRL        0x0020  // Clock Recovery Locked status
#define RFM12_ATGL       0x0010  // Toggling in each AFC Cycle
#define RFM12_OFFS_SIGN  0x0008  // Measured Offset Frequency Sign Value 1='+', 0='-' 
#define RFM12_OFFS       0x0004  // Measured offset Frequency value (3 bits) 
#define RFM12_OFFS_MASK  0x0003  // Measured offset mask

#define isRGITFFIT    (1 << 7)
#define isPOR         (1 << 6)
#define isRGURFFOV    (1 << 5)
#define isWKUP        (1 << 4)
#define isEXT         (1 << 3)
#define isLBD         (1 << 2)
#define isFFEM        (1 << 1)
#define isATS         (1)
#define isRSSI        (1 << 7)
#define isDQD         (1 << 6)
#define isCRL         (1 << 5)
#define isATGL        (1 << 4)

#define RFM12_STATUS	RFM12_STATUS_READ_BASE


#endif // RFM12_H
