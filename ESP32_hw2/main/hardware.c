#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "OSX_RPI_ESP.h"

#include "interface.h"
#include "RFM12.h"

#if(CONFIG_LED_RGB)
#include "led_rgb.h"
#endif

#if defined(__linux__) || OSX
	#include "interface_materiel.h"
// #warning "defined __linux__  || OSX"
#else	// ESP32
	#include <stdarg.h>
#endif

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* defines */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*  constantes */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
#if defined(__linux__) || OSX
#else
// static const char *TAG = "hard";
#endif
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*  variables externes */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
// extern const char *type[];
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*  variables locales */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*  fonctions locales */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*  fonctions globales */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
#if defined(__linux__) || OSX

#else	// ESP32

#endif

// NOTE vérouiller le mutex d'accès aux structures de mise à jour

/*==========================================================*/
/* Fonction appelée suite à modification d'un objet "button" dans la page index.html */ /* 
	- peut utiliser les structures _etatObj pour modifier une autre objet (ex: objet "P") */
/*==========================================================*/
void hardBouton(_etatObj *this, _etatObj *premiere) {
// 	INFOA("hardBouton ID %s Etat: %s", this->id, this->intValue == 1? "Marche" : "Arret")
	char temp[LONG_INNER] = "";
// 	_etatObj *ptrCtrl = premiere;
	char dataRFM12[SIZE_DATA] = {0};
	if(strcmp(this->id, "btn1") == 0) {
		snprintf(temp, MAX_INNER, "%s", this->intValue == 1? "Marche ?" : "Arret ?");
		strncpy(this->label_obj->innerHTML, temp, MAX_INNER);
// INFOE("envoi message module %s", S_RFM_1);
		strcpy(dataRFM12, S_RFM_1);
		for(int i=0; i < SIZE_DATA-5; i++) {
			dataRFM12[5+i] = 0x31 + i;
		}
		envoiMessRFM12(dataRFM12);
	}
	else if(strcmp(this->id, "btn2") == 0) {
// INFOE("envoi message module %s", S_RFM_2);
		snprintf(temp, MAX_INNER, "%s", this->intValue == 1? "Marche ?" : "Arret ?");
		strncpy(this->label_obj->innerHTML, temp, MAX_INNER);
		strcpy(dataRFM12, S_RFM_2);
		envoiMessRFM12(dataRFM12);
	}
}

/*==========================================================*/
/* Fonction appelée suite à modification d'un objet "range" dans la page index.html */ /* 
	- peut utiliser les structures _etatObj pour modifier une autre objet (ex: objet "P") */
/*==========================================================*/
void hardRange(_etatObj *this, _etatObj *premiere) {
// 	INFOA("hardRange ID %s Valeur: %i", this->id, this->intValue)
	char temp[LONG_INNER] = "";
	_etatObj *ptrCtrl = premiere;
	while(ptrCtrl != NULL) {
		if(strcmp(this->id, "rng1") == 0) {
			if(strcmp(ptrCtrl->id, "text1") == 0) {
				snprintf(temp, MAX_INNER, "Range: %i", this->intValue);
				strncpy(ptrCtrl->charValue, temp, MAX_INNER);
				break;
			}
		}
		ptrCtrl = ptrCtrl->suiv;
	}
}

/*==========================================================*/
/* Fonction appelée suite à modification d'un objet "color" dans la page index.html */ /* 
	- peut utiliser les structures _etatObj pour modifier une autre objet (ex: objet "P") */
/*==========================================================*/
void hardColor(_etatObj *this, _etatObj *premiere) {
// 	INFOA("hardColor ID %s Couleur: %s", this->id, this->charValue)
#if defined(__linux__) || OSX
	char temp[LONG_INNER] = "";
#endif
	int len;
	_etatObj *ptrCtrl = premiere;
	while(ptrCtrl != NULL) {
		if(strcmp(this->id, "col1") == 0) {
			if(strcmp(ptrCtrl->id, "text1") == 0) {
#if defined(__linux__) || OSX
				snprintf(temp, MAX_INNER, "Color: %s", this->charValue);
				strncpy(ptrCtrl->charValue, temp, MAX_INNER);
#else	// ESP32
				len = strlen(this->charValue);
				strncpy(ptrCtrl->charValue, "Color: ", MAX_INNER);
				strncpy(ptrCtrl->charValue + len, this->charValue, MAX_INNER - len);
	#if(CONFIG_LED_RGB)
				int r, g, b;
				sscanf(this->charValue, "#%02X%02X%02X", &r, &g, &b);
				rgb_t rgb = {r, g, b};
				rgb_esp_rgb(&rgb);
	#endif
#endif
				break;
			}
		}
		ptrCtrl = ptrCtrl->suiv;
	}
}

/*==========================================================*/
/* Fonction appelée suite à modification d'un objet "radio" dans la page index.html */ /* 
	- peut utiliser les structures _etatObj pour modifier une autre objet (ex: objet "P") */
/*==========================================================*/
void hardRadio(_etatObj *this, _etatObj *premiere) {
#if defined(__linux__) || OSX
// 	INFOA("hardRadio ID %s Etat: %s charValue %s innerHTML %s", this->id, this->intValue == 1? "On" : "Off", this->charValue, this->innerHTML)
	char temp[LONG_INNER] = "";
	_etatObj *ptrCtrl = premiere;
	while(ptrCtrl != NULL) {
		if(strcmp(this->charValue, "Radio") == 0) {
			if(strcmp(ptrCtrl->id, "text1") == 0) {
				snprintf(temp, MAX_INNER, "%s: %s", this->charValue, this->innerHTML);
				strncpy(ptrCtrl->charValue, temp, MAX_INNER);
				break;
			}
		}
		ptrCtrl = ptrCtrl->suiv;
	}
#else	// ESP32

#endif
}

/*==========================================================*/
/* Fonction appelée suite à modification d'un objet "checkbox" dans la page index.html */ /* 
	- peut utiliser les structures _etatObj pour modifier une autre objet (ex: objet "P") */
/*==========================================================*/
void hardCheckbox(_etatObj *this, _etatObj *premiere) {
#if defined(__linux__) || OSX
// 	INFOA("hardCheckbox ID %s Etat: %s charValue %s innerHTML %s", this->id, this->intValue == 1? "On" : "Off", this->charValue, this->innerHTML)
	char temp[LONG_INNER] = "";
	char temp1[LONG_INNER] = "";
	_etatObj *ptrCtrl = premiere;
	while(ptrCtrl != NULL) {
		if(strcmp(ptrCtrl->charValue, "Checkbox") == 0) {
			snprintf(temp1, MAX_INNER, "%s [%i] ", ptrCtrl->innerHTML, ptrCtrl->intValue);
			strncat(temp, temp1, MAX_INNER);
		}
		ptrCtrl = ptrCtrl->suiv;
	}	
	ptrCtrl = premiere;
	while(ptrCtrl != NULL) {
		if(strcmp(ptrCtrl->id, "text1") == 0) {
// INFOD("\n%s", temp)
			strncpy(ptrCtrl->charValue, temp, MAX_INNER);
			break;
		}
		ptrCtrl = ptrCtrl->suiv;
	}
#else	// ESP32

#endif
}

/*==========================================================*/
/* Fonction appelée suite à reception RFM 12 */ /* 
	- peut utiliser les structures _etatObj pour modifier un autre objet (ex: objet "P") */
/*==========================================================*/
void hardRFM12(_etatObj *premiere, unsigned char *bufRX) {
	char temp[LONG_INNER] = "";
#if defined(__linux__) || OSX
	uint recu = 0;
	recu = bufRX[0];
	printf("hardRFM12[%i] recu : %i octets : ", __LINE__, recu);
	for(int i=0; i<SIZE_DATA; i++) {
		printf("0x%02X ", bufRX[i+1]);
	}
	printf("\n");
#endif

		// ======== module S_RFM_1	: LED
	if(strncmp((char*)bufRX+RFM_ID, S_RFM_1, 5) == 0)	{
		_etatObj *ptrCtrl = premiere;
		while(ptrCtrl != NULL) {
				if(strcmp(ptrCtrl->id, "btn1") == 0) {
					ptrCtrl->intValue = bufRX[RFM_DATA] ? 1 : 0;
				}
				if(strcmp(ptrCtrl->id, "p1") == 0) {
					snprintf(temp, MAX_INNER, "%s", bufRX[RFM_DATA] ? "En Marche" : "A l'Arret");
					strncpy(ptrCtrl->innerHTML, temp, MAX_INNER);
				}
			ptrCtrl = ptrCtrl->suiv;
		}
		return;
	}
		// ======== module S_RFM_2	: DS18B20
	else if(strncmp((char*)bufRX+RFM_ID, S_RFM_2, 5) == 0)	{
		float result = 0.0f;
		int16_t raw = (bufRX[RFM_DATA+1] << 8) | bufRX[RFM_DATA];
		result = raw / 16.0f;
		snprintf(temp, MAX_INNER, "%0.3f°C", result);
	}
		// ======== autre module
	else {
		snprintf(temp, MAX_INNER, "module %c%c%c%c%c", bufRX[1], bufRX[2], bufRX[3], bufRX[4], bufRX[5]);
	}

	// affiche le resultat dans "text1"
	_etatObj *ptrCtrl = premiere;
	while(ptrCtrl != NULL) {
			if(strcmp(ptrCtrl->id, "text1") == 0) {
				strncpy(ptrCtrl->charValue, temp, MAX_INNER);
				break;
			}
		ptrCtrl = ptrCtrl->suiv;
	}
}

/*==========================================================*/
/* Fonction appelée suite à lecture capteur interne */ /* 
	- peut utiliser les structures _etatObj pour modifier un autre objet (ex: objet "P") */
/*==========================================================*/
void hardDHT(_etatObj *premiere, float temperature, float humidite, int err) {
	char temp[LONG_INNER] = "";
#if defined(__linux__) || OSX
	return;
#endif
	snprintf(temp, MAX_INNER, "%0.3f °C \\nHum: %0.3f %%", temperature, humidite);
	// affiche le resultat dans "text1"
	_etatObj *ptrCtrl = premiere;
	while(ptrCtrl != NULL) {
			if(strcmp(ptrCtrl->id, "text2") == 0) {
				strncpy(ptrCtrl->charValue, temp, MAX_INNER);
				break;
			}
		ptrCtrl = ptrCtrl->suiv;
	}
}

/*==========================================================*/
/* Fonction appelée suite à lecture capteur interne */ /* 
	- peut utiliser les structures _etatObj pour modifier un autre objet (ex: objet "P") */
/*==========================================================*/
void hardDS18B20(_etatObj *premiere, float temperature) {
	char temp[LONG_INNER] = "";
#if defined(__linux__) || OSX
	return;
#endif
	snprintf(temp, MAX_INNER, "%0.3f°C", temperature);
	// affiche le resultat dans "text1"
	_etatObj *ptrCtrl = premiere;
	while(ptrCtrl != NULL) {
			if(strcmp(ptrCtrl->id, "text2") == 0) {
				strncpy(ptrCtrl->charValue, temp, MAX_INNER);
				break;
			}
		ptrCtrl = ptrCtrl->suiv;
	}
}

/*==========================================================*/
/*  */ /*  */
/*==========================================================*/

