#ifndef TMP75_H
#define TMP75_H

#define adresse_TMP75 0x49	/* adresse de base */
/* registres à appeler pour acceder aux registres internes */ 
#define Reg_Lecture			0x00	/* lire la temperature (2 octets à lire) */
#define Reg_config			0x01	/* REGISTRE CONFIG (+1 octet) */	
	/*
		D7		D6		D5		D4		D3		D2		D1		D0
		OS		R1		R0		F1		F0		POL		TM		SD
	*/	
	#define TMP75_SD		0x01	/* SHUTDOWN MODE */
	#define TMP75_TM		0x02	/* THERMOSTAT MODE */
	#define TMP75_POL		0x04	/* POLARITY */
	/* FAULT QUEUE */
	#define TMP75_F_1		0x00
	#define TMP75_F_2		0x08
	#define TMP75_F_4		0x10
	#define TMP75_F_6		0x18
	#define TMP75_OS		0x80	/* ONE-SHOT */
	/* resolution de lecture */
	#define PREC_0_0625	0
	#define PREC_0_125	1
	#define PREC_0_25		2
	#define PREC_0_5		3
#define Reg_Temp_basse	0x02	/* seuil bas R/W (+2 octets) */
#define Reg_Temp_haute	0x03	/* seuil haut R/W (+2 octets) */

int tmp75_init(int resolution);
int tmp75_close(void);
int tmp75_lect(float *temperature);

#endif