/* interface.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "cJSON.h"

#include "OSX_RPI_ESP.h"

#include "interface.h"
#include "hardware.h"


#if defined(__linux__) || OSX
#else		// EPS32
#endif


////////////////////////////////////////////////////////////////////////////
//	define
////////////////////////////////////////////////////////////////////////////
#define	LOCK_MUTEX_INTERFACE		pthread_mutex_lock(&mutexInterface); // INFOA("\t interface LOCK_MUTEX_INTERFACE");
#define	UNLOCK_MUTEX_INTERFACE	pthread_mutex_unlock(&mutexInterface); // INFOA("\t interface UNLOCK_MUTEX_INTERFACE");

////////////////////////////////////////////////////////////////////////////
//	variables externes
////////////////////////////////////////////////////////////////////////////
extern pthread_mutex_t mutexInterface;

////////////////////////////////////////////////////////////////////////////
//	variables locales
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
//  constantes
////////////////////////////////////////////////////////////////////////////
#if defined(__linux__) || OSX
#else		// EPS32
// static const char *TAG = "interface";
#endif

//==========================================================
/* Constantes liés aux énumération pour disposer des chaines de caractères */ /* 
		Peuvent être utilisées en dehors du fichier interface.c en extern */
/*==========================================================*/
const char *tag[]	= {"fct", "label", "input", "br", "p", "textarea"};

const char *type[]	= {"button", "range", "color", "radio", "checkbox", "textep", "textearea"};

const char *param[] = {"innerHTML", "htmlFor", "className", "type", "id", "value", "hidden", "name", "checked"};

////////////////////////////////////////////////////////////////////////////
//  variable globales
////////////////////////////////////////////////////////////////////////////
int idx = 0;		// index pour créer un objet json virtuel pour chaque objet lors de la création de la page HTML

////////////////////////////////////////////////////////////////////////////
//  fonctions locales
////////////////////////////////////////////////////////////////////////////

//==========================================================
/* Ajoute dans le buffer "dest" les chaines formatées */ /* 
		Utilise un buffer temporaire "buf" dont la taille doit être ajusté au besoin */
/*==========================================================*/
// !!! Attention à la taille des buffers
#define AJOUT_STR(dest, buf, fmt, ...) MaConcat(buf, fmt, __VA_ARGS__); strcat(dest, buf);								
void MaConcat(char* dest_str, const char * format, ...) {
  va_list args;
  va_start(args, format);
  vsprintf(dest_str, format, args);
  va_end(args);
}

// NOTE : les fonctions "CREER_xxx" retourne un pointeur "pStructCtrl" vers l'objet nouvellement créé
//==========================================================
/* Crée un objet P (paragraphe) */ /* 
	- id = identifiant unique
	- innerHTML = contenu affiché
	- className = style utilisé
	- lien avec l'objet précedent
*/
/*==========================================================*/
#define CREER_P(id, innerHTML, className, lienObjPrec) pStructCtrl=creerP(pStructCtrl, id, innerHTML, className, lienObjPrec)
#define vP nouvCtrl->parametre.p
void  *creerP(_controle *prec, char *id, char *innerHTML, char *className, int lienObjPrec) {
	_controle *nouvCtrl = malloc(sizeof(_controle));
	if(!nouvCtrl) {    // Si l'allocation a échoué.
		INFOE("l'allocation a échoué")
		return NULL;
	}	
// 	INFOA("alloc %p", nouvCtrl)
	nouvCtrl->idFct = 0;
	nouvCtrl->tagName = P;
	vP.lienObjPrec = lienObjPrec;	
	snprintf(nouvCtrl->nomFonction, MAX_STR, "obj_%i", idx);
	strncpy(vP.id, id, MAX_STR);
	idx++;
	strncpy(vP.innerHTML, innerHTML, MAX_INNER);
	strncpy(vP.className, className, MAX_STR);
	nouvCtrl->suiv = NULL;       // Le pointeur pointe sur le dernier élément.
	prec->suiv = nouvCtrl;
	return nouvCtrl;
}

//==========================================================
/* Crée un objet radio ou checkbox */ /* 
	- id = identifiant unique
	- name = nom commun au radio ou checkbox d'un même groupe
	- value = nom spécifique à chaque bouton
	- checked = état du bouton */
/*==========================================================*/
#define CREER_RADIO(id, name, value, checked) pStructCtrl=creerRadCheck(pStructCtrl, 0, id, name, value, checked)
#define CREER_CHECKBOX(id, name, value, checked) pStructCtrl=creerRadCheck(pStructCtrl, 1, id, name, value, checked)
#define vRadio nouvCtrl->parametre.radio
#define vCheckbox nouvCtrl->parametre.checkbox
void  *creerRadCheck(_controle *prec, int type, char *id, char *name, char *value, int checked ) {
	_controle *nouvCtrl = malloc(sizeof(_controle));
	if(!nouvCtrl) {    // Si l'allocation a échoué.
		INFOE("l'allocation a échoué")
		return NULL;
	}	
// // 	INFOA("alloc %p", nouvCtrl)
	nouvCtrl->idFct = 0;
	nouvCtrl->tagName = INPUT;
	snprintf(nouvCtrl->nomFonction, MAX_STR, "obj_%i", idx);
	idx++;
	if(type == 0) {
		strncpy(vRadio.id, id, MAX_STR);
		vRadio.type = RADIO;
		strncpy(vRadio.name, name, MAX_STR);
		strncpy(vRadio.value, value, MAX_STR);
		vRadio.checked = checked;
	}
	else {
		strncpy(vCheckbox.id, id, MAX_STR);
		vCheckbox.type = CHECKBOX;
		strncpy(vCheckbox.name, name, MAX_STR);
		strncpy(vCheckbox.value, value, MAX_STR);
		vCheckbox.checked = checked;
	}
	
	nouvCtrl->suiv = NULL;       // Le pointeur pointe sur le dernier élément.
	prec->suiv = nouvCtrl;
	return nouvCtrl;
}

//==========================================================
/* Crée un objet color) */ /* 
	- id = identifiant unique
	- value = valeur de la couleur au format #xxxxxx */
/*==========================================================*/
#define CREER_COLOR(id, value) pStructCtrl=creerColor(pStructCtrl, id, value)
#define vColor nouvCtrl->parametre.color
void  *creerColor(_controle *prec, char *id, char *value) {
	_controle *nouvCtrl = malloc(sizeof(_controle));
	if(!nouvCtrl) {    // Si l'allocation a échoué.
		INFOE("l'allocation a échoué")
		return NULL;
	}	
// 	INFOA("alloc %p", nouvCtrl)
	nouvCtrl->idFct = 0;
	nouvCtrl->tagName = INPUT;
	snprintf(nouvCtrl->nomFonction, MAX_STR, "obj_%i", idx);
	strncpy(vColor.id, id, MAX_STR);
	idx++;
	vColor.type = COLOR;
	strncpy(vColor.value, value, MAX_STR);
	
	nouvCtrl->suiv = NULL;       // Le pointeur pointe sur le dernier élément.
	prec->suiv = nouvCtrl;
	return nouvCtrl;
}

//==========================================================
/* Crée un objet range */ /* 
	- id = identifiant unique
	- value = valeur de l'objet */
/*==========================================================*/
#define CREER_RANGE(id, value) pStructCtrl=creerRange(pStructCtrl, id, value)
#define vRange nouvCtrl->parametre.range
void  *creerRange(_controle *prec, char *id, int value) {
	_controle *nouvCtrl = malloc(sizeof(_controle));
	if(!nouvCtrl) {    // Si l'allocation a échoué.
		INFOE("l'allocation a échoué")
		return NULL;
	}	
// 	INFOA("alloc %p", nouvCtrl)
	nouvCtrl->idFct = 0;
	nouvCtrl->tagName = INPUT;
	snprintf(nouvCtrl->nomFonction, MAX_STR, "obj_%i", idx);
	strncpy(vRange.id, id, MAX_STR);
	idx++;
	vRange.type = RANGE;
	vRange.value = value;
	
	nouvCtrl->suiv = NULL;       // Le pointeur pointe sur le dernier élément.
	prec->suiv = nouvCtrl;
	return nouvCtrl;
}

//==========================================================
/* Crée un objet button */ /* 
	- id = identifiant unique
	- value = 0 ou 1 reflète l'état du bouton
	- hidden = utilisé pour masquer le bouton si utilisation d'une image dans le label */
/*==========================================================*/
#define CREER_BOUTON(id, value, hidden) pStructCtrl=creerBouton(pStructCtrl, id, value, hidden)
#define vBouton nouvCtrl->parametre.button
void  *creerBouton(_controle *prec, char *id, int value, int hidden) {
	_controle *nouvCtrl = malloc(sizeof(_controle));
	if(!nouvCtrl) {    // Si l'allocation a échoué.
		INFOE("l'allocation a échoué")
		return NULL;
	}	
// 	INFOA("alloc %p", nouvCtrl)
	nouvCtrl->idFct = 0;
	nouvCtrl->tagName = INPUT;
	snprintf(nouvCtrl->nomFonction, MAX_STR, "obj_%i", idx);
	idx++;
	vBouton.type = BUTTON;
	strncpy(vBouton.id, id, MAX_STR);
	vBouton.value = value;
	vBouton.hidden = hidden;
	nouvCtrl->suiv = NULL;       // Le pointeur pointe sur le dernier élément.
	prec->suiv = nouvCtrl;
	return nouvCtrl;
}

//==========================================================
/* Crée un retour à la ligne entre les objets */ /*  */
/*==========================================================*/
#define CREER_BR pStructCtrl=creerBr(pStructCtrl)
void  *creerBr(_controle *prec) {
	_controle *nouvCtrl = malloc(sizeof(_controle));
	if(!nouvCtrl) {    // Si l'allocation a échoué.
		INFOE("l'allocation a échoué")
		return NULL;
	}	
// 	INFOA("alloc %p", nouvCtrl)
	snprintf(nouvCtrl->nomFonction, MAX_STR, "obj_%i", idx);
	idx++;
	nouvCtrl->idFct = 0;
	nouvCtrl->tagName = _BR;
	nouvCtrl->suiv = NULL;       // Le pointeur pointe sur le dernier élément.
	prec->suiv = nouvCtrl;
	return nouvCtrl;
}

//==========================================================
/* Crée un objet label */ /* 
	- CREER_LABEL => innerHTML = contenu affiché
		ou
	- CREER_LABEL_IMG => image = image affiché avec l'objet auquel s'applique le label
	- htmlFor = id de l'objet auquel s'applique le label
	- className = style utilisé */
/*==========================================================*/
/* NOTE IMPORTANTE */ /*
	pour les types "INPUT" placer CREER_LABEL après la création de l'INPUT pour
	acccéder au LABEL lorsque que INPUT est modifié
*/
#define CREER_LABEL(innerHTML, htmlFor, className) pStructCtrl=creerLabel(0, pStructCtrl, htmlFor, innerHTML, className)
#define CREER_LABEL_IMG(image, htmlFor, className) pStructCtrl=creerLabel(1, pStructCtrl, htmlFor, image, className)
#define vlabel nouvCtrl->parametre.label
void  *creerLabel(int img, _controle *prec, char *innerHTML, char *htmlFor, char *className) {
	_controle *nouvCtrl = malloc(sizeof(_controle));
	if(!nouvCtrl) {    // Si l'allocation a échoué.
		INFOE("l'allocation a échoué")
		return NULL;
	}	
// 	INFOA("alloc %p", nouvCtrl)
	nouvCtrl->idFct = 0;
	nouvCtrl->tagName = LABEL;
	snprintf(nouvCtrl->nomFonction, MAX_STR, "obj_%i", idx);
	idx++;
	if(!img) {
		strncpy(vlabel.innerHTML, innerHTML, MAX_INNER);
	}
	else {
		char img[LONG_INNER*3] = "";
		snprintf(img, MAX_INNER * 3 * sizeof(char), "<img id=\\\"img_%s\\\" src=\\\"%s\\\" width=\\\"60\\\">", htmlFor, innerHTML);
		strncpy(vlabel.innerHTML, img, MAX_INNER * 3);
	}
		
	strncpy(vlabel.htmlFor, htmlFor, MAX_STR);
	strncpy(vlabel.className, className, MAX_STR);
	nouvCtrl->suiv = NULL;       // Le pointeur pointe sur le dernier élément.
	prec->suiv = nouvCtrl;
	return nouvCtrl;
}

//==========================================================
/* Crée un objet textarea */ /* 
	- id = identifiant unique
	- value = contenu affiché */
/*==========================================================*/
#define CREER_TEXTAREA(id, value) pStructCtrl=creerTextarea(pStructCtrl, id, value)
#define vTextarea nouvCtrl->parametre.textarea
void  *creerTextarea(_controle *prec, char *id, char *value) {
	_controle *nouvCtrl = malloc(sizeof(_controle));
	if(!nouvCtrl) {    // Si l'allocation a échoué.
		INFOE("l'allocation a échoué")
		return NULL;
	}	
// 	INFOA("alloc %p", nouvCtrl)
	nouvCtrl->idFct = 0;
	nouvCtrl->tagName = TEXTEAREA;
	snprintf(nouvCtrl->nomFonction, MAX_STR, "obj_%i", idx);
	idx++;
	strncpy(vTextarea.id, id, MAX_STR);
	strncpy(vTextarea.value, value, MAX_INNER);
	nouvCtrl->suiv = NULL;       // Le pointeur pointe sur le dernier élément.
	prec->suiv = nouvCtrl;
	return nouvCtrl;
}

//==========================================================
/* Crée une "fonction" qui regroupe des objets */ /* 
	- nom = nom de la fonction */
/*==========================================================*/
#define CREER_FONCTION(nom) pStructCtrl=creerFonction(pStructCtrl, nom)
void *creerFonction(_controle *prec, char *nom) {
	_controle *nouvFct = malloc(sizeof(_controle));
	if(!nouvFct) {    // Si l'allocation a échoué.
		INFOE("l'allocation a échoué")
		return NULL;
	}
// 	INFOA("alloc %p", nouvFct)
	nouvFct->idFct = 1;
	strncpy(nouvFct->nomFonction, nom, MAX_STR);
	nouvFct->tagName = FCT;
	nouvFct->suiv = NULL;
	prec->suiv = nouvFct;      // Le pointeur de la fonction précédente pointe sur la nouvelle fonction.
// 	INFOA("     fct: %s nouvFct %p prec %p prec->suiv %p", nouvFct->nomFonction, nouvFct, prec, prec->suiv)
	return nouvFct;
}

////////////////////////////////////////////////////////////////////////////
//  fonctions globales
////////////////////////////////////////////////////////////////////////////
//==========================================================
/* crée un json pour retourner l'état matériel au client http */ /* 
	- utilise un mutex pour éviter les collisions entre appels  de mise à jour
	- retourne le json pour la structure serveur.premiere (serveur.c)
	- fait appel aux structures _etatObj*/
/*==========================================================*/
void creerJsonEtatHard(char *jsonRetour, _etatObj *premiere) {
// 	INFOD("========= création JSON état hardware ===========")
	_etatObj *ptrEtatObj = premiere;
	char buf[128] = "";  
	jsonRetour[0] = '\0';
	
// ------------------------------------------------------------------- début de la zone critique -----------------				
	LOCK_MUTEX_INTERFACE /* On verrouille le mutex */

	AJOUT_STR(jsonRetour, buf, "{\n", '\0');									
	while(ptrEtatObj != NULL) {
		AJOUT_STR(jsonRetour, buf, " \"%s\": {\n", ptrEtatObj->id)
		switch(ptrEtatObj->type) {
			case BUTTON:
				AJOUT_STR(jsonRetour, buf, "  \"%s\": \"%s\",\n", param[TYPE], type[BUTTON])
				AJOUT_STR(jsonRetour, buf, "  \"%s\": %i\n", param[VALUE], ptrEtatObj->intValue)
				break;
			case COLOR:
				AJOUT_STR(jsonRetour, buf, "  \"%s\": \"%s\",\n", param[TYPE], type[COLOR])
				AJOUT_STR(jsonRetour, buf, "  \"%s\": \"%s\"\n", param[VALUE], ptrEtatObj->charValue)
				break;
			case RANGE:
				AJOUT_STR(jsonRetour, buf, "  \"%s\": \"%s\",\n", param[TYPE], type[RANGE])
				AJOUT_STR(jsonRetour, buf, "  \"%s\": %i\n", param[VALUE], ptrEtatObj->intValue)
				break;
			case RADIO:
				AJOUT_STR(jsonRetour, buf, "  \"%s\": \"%s\",\n", param[TYPE], type[RADIO])
				AJOUT_STR(jsonRetour, buf, "  \"%s\": %i\n", param[CHECKED], ptrEtatObj->intValue)
				break;
			case CHECKBOX:
				AJOUT_STR(jsonRetour, buf, "  \"%s\": \"%s\",\n", param[TYPE], type[CHECKBOX])
				AJOUT_STR(jsonRetour, buf, "  \"%s\": %i\n", param[CHECKED], ptrEtatObj->intValue)
				break;
			case TEXT_P:
				AJOUT_STR(jsonRetour, buf, "  \"%s\": \"%s\",\n", param[TYPE], type[TEXT_P])
				AJOUT_STR(jsonRetour, buf, "  \"%s\": \"%s\"\n", param[INNERHTML], ptrEtatObj->innerHTML)		
				break;
			case TXTAREA:
				AJOUT_STR(jsonRetour, buf, "  \"%s\": \"%s\",\n", param[TYPE], type[TXTAREA])
				AJOUT_STR(jsonRetour, buf, "  \"%s\": \"%s\"\n", param[VALUE], ptrEtatObj->charValue)		
				break;
		}
		if(ptrEtatObj->suiv != NULL) {
			AJOUT_STR(jsonRetour, buf, " },\n", '\0');
		}
		ptrEtatObj = ptrEtatObj->suiv;
	};
	AJOUT_STR(jsonRetour, buf, " }\n}\n", '\0');

			UNLOCK_MUTEX_INTERFACE /* On déverrouille le mutex */
 // ------------------------------------------------------------- fin de la zone critique -------------------
// 	INFOD("\n%s", jsonRetour)
}

//==========================================================
/* Effectue la mise à jours des structures _etatObj suite à un apple du client http */ /*  */
// TODO - implémenter l'interface avec le matériel
/*==========================================================*/
void miseAJourWeb(char *jsonRecep, _etatObj *premiere)
{
// 	INFOD("=========== debut %s ===========", __FUNCTION__)
	cJSON *cData_json;
	cJSON *objId;
	cJSON *ObjVal;
// 	INFOD("jsonRecep \n%s", jsonRecep)
	cData_json = cJSON_Parse(jsonRecep);

	_etatObj *ptrCtrl = premiere;
	_etatObj *ptrObjModif = premiere->obj_modif;

// ------------------------------------------------------------------- début de la zone critique -----------------				
	LOCK_MUTEX_INTERFACE /* On verrouille le mutex */
	
	while(ptrCtrl != NULL) {
		objId = cJSON_GetObjectItem(cData_json, ptrCtrl->id);
		if(objId != NULL) {
			switch(ptrCtrl->type) {
				case BUTTON:
					ObjVal = cJSON_GetObjectItem(objId, "value" );
					ptrCtrl->intValue = (int)ObjVal->valuedouble;
					ptrObjModif = ptrCtrl;
					break;
				case COLOR:
					ObjVal = cJSON_GetObjectItem(objId, "value" );
					strcpy(ptrCtrl->charValue, ObjVal->valuestring);
					ptrObjModif = ptrCtrl;
					break;
				case RANGE:
					ObjVal = cJSON_GetObjectItem(objId, "value" );
					ptrCtrl->intValue = (int)ObjVal->valuedouble;
					ptrObjModif = ptrCtrl;
					break;
				case RADIO:
					ObjVal = cJSON_GetObjectItem(objId, "checked" );
					_etatObj *ptrRadio = premiere;
					while(ptrRadio != NULL) {
						if(strcmp(ptrCtrl->charValue, ptrRadio->charValue) == 0) {
					 		ptrRadio->intValue = 0;
						}
						ptrRadio = ptrRadio->suiv;
					}
					ptrCtrl->intValue = cJSON_IsTrue(ObjVal);
					ptrObjModif = ptrCtrl;
					break;
				case CHECKBOX:
					ObjVal = cJSON_GetObjectItem(objId, "checked" );
					ptrCtrl->intValue = cJSON_IsTrue(ObjVal);
					ptrObjModif = ptrCtrl;
					break;
			}			
		}	
		ptrCtrl = ptrCtrl->suiv;
	}

	premiere->obj_modif = ptrObjModif;
// INFOE("modif %s -- %s", premiere->obj_modif->id, ptrObjModif->id);


			UNLOCK_MUTEX_INTERFACE /* On déverrouille le mutex */
// ------------------------------------------------------------- fin de la zone critique -------------------
// 	INFOD("%s", bufInfo)

	cJSON_Delete(cData_json);
}

//==========================================================
/* Créer les structures _etatObj pour conserver l'état matériel et interagir avec */ /* 
	- fait appel à "creerPageHTML" pour l'initialisation
	- doit être appelé en debut de programme et transmis au serveur http
	- retourne le pointeur vers la première structure  */
/*==========================================================*/
void *creerStructMiseAJour(void) {
// 	INFOD("========= création structure mise à jour ===========")
	_etatObj *ptrCtrl = malloc(sizeof(_etatObj));
	if(!ptrCtrl) {    // Si l'allocation a échoué.
		INFOE("l'allocation a échoué")
		return NULL;
	}
	ptrCtrl->suiv = NULL;
	ptrCtrl->id[0] = '\0';		// pour utiliser la structure déjà créée
	ptrCtrl->obj_modif = NULL;
	ptrCtrl->label_obj = NULL;
	void *premiere = ptrCtrl;
	_etatObj *inputPrec = NULL;

	_controle *ptrObjHTML = creerPageHTML();
	if(ptrObjHTML == NULL) {
		return NULL;
	}
	
	void *premiereHTML = ptrObjHTML;
	
	while(ptrObjHTML != NULL) {
		if(ptrObjHTML->idFct == 1) {
			ptrObjHTML = ptrObjHTML->suiv;
		}
		else {
			switch (ptrObjHTML->tagName) {
				case LABEL:
					break; 
				case _BR:
					break;
				case INPUT:
					switch (ptrObjHTML->parametre.button.type) {
						case BUTTON:
							strcpy(ptrCtrl->id, ptrObjHTML->parametre.button.id);
							ptrCtrl->type = BUTTON;
							ptrCtrl->intValue = ptrObjHTML->parametre.button.value;
							// NOTE
							ptrCtrl->fonctionCtrl = hardBouton;
							break;
						case COLOR:
							strcpy(ptrCtrl->id, ptrObjHTML->parametre.color.id);
							ptrCtrl->type = COLOR;
							strcpy(ptrCtrl->charValue, ptrObjHTML->parametre.color.value);
							// NOTE
							ptrCtrl->fonctionCtrl = hardColor;
							break;
						case RANGE:
							strcpy(ptrCtrl->id, ptrObjHTML->parametre.range.id);
							ptrCtrl->type = RANGE;
							ptrCtrl->intValue = ptrObjHTML->parametre.range.value;
							// NOTE
							ptrCtrl->fonctionCtrl = hardRange;
							break;
						case RADIO:
							strcpy(ptrCtrl->id, ptrObjHTML->parametre.radio.id);
							ptrCtrl->type = RADIO;
							ptrCtrl->intValue = ptrObjHTML->parametre.radio.checked;
							strcpy(ptrCtrl->charValue, ptrObjHTML->parametre.radio.name);
							strcpy(ptrCtrl->innerHTML, ptrObjHTML->parametre.radio.value);
							// NOTE
							ptrCtrl->fonctionCtrl = hardRadio;
							break;
						case CHECKBOX:
							strcpy(ptrCtrl->id, ptrObjHTML->parametre.checkbox.id);
							ptrCtrl->type = CHECKBOX;
							ptrCtrl->intValue = ptrObjHTML->parametre.checkbox.checked;
							strcpy(ptrCtrl->charValue, ptrObjHTML->parametre.checkbox.name);
							strcpy(ptrCtrl->innerHTML, ptrObjHTML->parametre.checkbox.value);
							// NOTE
							ptrCtrl->fonctionCtrl = hardCheckbox;
							break;
					}			

					// pour accéder au label (s'il existe) afin de le modifier si INPUT est modifié
					if(strcmp(ptrObjHTML->suiv->parametre.label.htmlFor, ptrObjHTML->parametre.button.id) == 0) {
INFO("label %s pour %s", ptrObjHTML->suiv->parametre.label.htmlFor, ptrObjHTML->parametre.button.id);
						ptrCtrl->label_obj = ptrCtrl->suiv;
					}
					//	pour lier un paragraphe à l'INPUT
					inputPrec = ptrCtrl;

				break;
				case P:
					strcpy(ptrCtrl->id, ptrObjHTML->parametre.p.id);
					ptrCtrl->type = TEXT_P;
					strcpy(ptrCtrl->innerHTML, ptrObjHTML->parametre.p.innerHTML);
					// si le paragraphe est lié à un INPUT
					if(ptrObjHTML->parametre.p.lienObjPrec) {
						inputPrec->label_obj = ptrCtrl;
INFO("p %s pour input %s", ptrCtrl->id, inputPrec->id);
					}
					break;
				case TEXTEAREA:
					strcpy(ptrCtrl->id, ptrObjHTML->parametre.textarea.id);
					ptrCtrl->type = TXTAREA;
					strcpy(ptrCtrl->charValue, ptrObjHTML->parametre.textarea.value);					
					break;
			}
			ptrObjHTML = ptrObjHTML->suiv;
			if((ptrObjHTML != NULL) && (ptrCtrl->id[0] != '\0') && (\
														(ptrObjHTML->tagName == INPUT) || \
												(ptrObjHTML->tagName == TEXTEAREA) || \
																	(ptrObjHTML->tagName == P) )) {
				ptrCtrl->suiv = malloc(sizeof(_etatObj));
				if(!ptrCtrl->suiv) {    // Si l'allocation a échoué.
					INFOE("l'allocation a échoué")
					return NULL;
				}
				ptrCtrl = ptrCtrl->suiv;
				ptrCtrl->suiv = NULL;
			}
		}
	};
	
	freePageHTML(premiereHTML);
	return premiere;
}

//==========================================================
/* Construit dans le buffer json les données pour créer les objets de la page index.html*/ /* 
	- est appelé par le client http lorsque la page index.hmtl est chargée
	- ait appel à "creerPageHTML" pour l'initialisation  */
/*==========================================================*/
void creerJsonPageHTML(char json[]) {
// 	INFOD("========= creerJsonPageHTML ===========")
	_controle *ptrFct = creerPageHTML();
	void *premiere = ptrFct;
	char buf[100] = "";
	
	json[0] = '\0';
	AJOUT_STR(json, buf, "{\n", "");									
	while(ptrFct != NULL) {
		if(ptrFct->idFct) {
			AJOUT_STR(json, buf, "  \"%s\": {\n", ptrFct->nomFonction)
			ptrFct = ptrFct->suiv;
		}
		else {
			AJOUT_STR(json, buf, "    \"%s\": {\n", ptrFct->nomFonction)
			switch (ptrFct->tagName) {
				case LABEL:
					AJOUT_STR(json, buf, "      \"tagName\": \"%s\",\n", tag[LABEL])
					AJOUT_STR(json, buf, "      \"%s\": \"%s\",\n", param[INNERHTML], ptrFct->parametre.label.innerHTML)		
					AJOUT_STR(json, buf, "      \"%s\": \"%s\",\n", param[HTMLFOR], ptrFct->parametre.label.htmlFor)		
					AJOUT_STR(json, buf, "      \"%s\": \"%s\"\n", param[CLASSNAME], ptrFct->parametre.label.className)		
					break; 
				case _BR:
					AJOUT_STR(json, buf, "      \"tagName\": \"%s\"\n", tag[_BR])				
					break;
				case INPUT:
					AJOUT_STR(json, buf, "      \"tagName\": \"%s\",\n", tag[INPUT])
					switch (ptrFct->parametre.button.type) {
						case BUTTON:
							AJOUT_STR(json, buf, "      \"%s\": \"%s\",\n", param[TYPE], type[BUTTON])
							AJOUT_STR(json, buf, "      \"%s\": \"%s\",\n", param[ID], ptrFct->parametre.button.id)
							AJOUT_STR(json, buf, "      \"%s\": %i,\n", param[VALUE], ptrFct->parametre.button.value)
							AJOUT_STR(json, buf, "      \"%s\": %i\n", param[HIDDEN], ptrFct->parametre.button.hidden)
							break;
						case COLOR:
							AJOUT_STR(json, buf, "      \"%s\": \"%s\",\n", param[TYPE], type[COLOR])
							AJOUT_STR(json, buf, "      \"%s\": \"%s\",\n", param[ID], ptrFct->parametre.color.id)
							AJOUT_STR(json, buf, "      \"%s\": \"%s\"\n", param[VALUE], ptrFct->parametre.color.value)
							break;
						case RANGE:
							AJOUT_STR(json, buf, "      \"%s\": \"%s\",\n", param[TYPE], type[RANGE])
							AJOUT_STR(json, buf, "      \"%s\": \"%s\",\n", param[ID], ptrFct->parametre.range.id)
							AJOUT_STR(json, buf, "      \"%s\": %i\n", param[VALUE], ptrFct->parametre.range.value)
							break;
						case RADIO:
							AJOUT_STR(json, buf, "      \"%s\": \"%s\",\n", param[TYPE], type[RADIO])
							AJOUT_STR(json, buf, "      \"%s\": \"%s\",\n", param[ID], ptrFct->parametre.radio.id)
							AJOUT_STR(json, buf, "      \"%s\": \"%s\",\n", param[NAME], ptrFct->parametre.radio.name)
							AJOUT_STR(json, buf, "      \"%s\": \"%s\",\n", param[VALUE], ptrFct->parametre.radio.value)
							AJOUT_STR(json, buf, "      \"%s\": %i\n", param[CHECKED], ptrFct->parametre.radio.checked)
							break;
						case CHECKBOX:
							AJOUT_STR(json, buf, "      \"%s\": \"%s\",\n", param[TYPE], type[CHECKBOX])
							AJOUT_STR(json, buf, "      \"%s\": \"%s\",\n", param[ID], ptrFct->parametre.checkbox.id)
							AJOUT_STR(json, buf, "      \"%s\": \"%s\",\n", param[NAME], ptrFct->parametre.checkbox.name)
							AJOUT_STR(json, buf, "      \"%s\": \"%s\",\n", param[VALUE], ptrFct->parametre.checkbox.name)
							AJOUT_STR(json, buf, "      \"%s\": %i\n", param[CHECKED], ptrFct->parametre.checkbox.checked)
							break;
					}			
				break;
				case P:
					AJOUT_STR(json, buf, "      \"tagName\": \"%s\",\n", tag[P])				
					AJOUT_STR(json, buf, "      \"%s\": \"%s\",\n", param[ID], ptrFct->parametre.p.id)
					AJOUT_STR(json, buf, "      \"%s\": \"%s\",\n", param[INNERHTML], ptrFct->parametre.p.innerHTML)		
					AJOUT_STR(json, buf, "      \"%s\": \"%s\"\n", param[CLASSNAME], ptrFct->parametre.p.className)		
					break;
				case TEXTEAREA:
					AJOUT_STR(json, buf, "      \"tagName\": \"%s\",\n", tag[TEXTEAREA])				
					AJOUT_STR(json, buf, "      \"%s\": \"%s\",\n", param[ID], ptrFct->parametre.textarea.id)
					AJOUT_STR(json, buf, "      \"%s\": \"%s\"\n", param[INNERHTML], ptrFct->parametre.textarea.value)		
					break;
			}
			ptrFct = ptrFct->suiv;
			if(ptrFct && ptrFct->idFct == 0) {
				AJOUT_STR(json, buf, "    },\n", "")
			}
			else {
				AJOUT_STR(json, buf, "    }\n", "")
			}
		}
		if(ptrFct && ptrFct->idFct) {
			AJOUT_STR(json, buf, "  },\n", "")
		}		
	};
	AJOUT_STR(json, buf, "  }\n}\n", "")
// 	INFOD("\n%s", json)
	
	freePageHTML(premiere);
}

//==========================================================
/* Libère la mémoire utilisé par la création de la page html */ /* 
	- à appeler après utilisation de "creerPageHTML" */
/*==========================================================*/
void freePageHTML(_controle *premiereFct) {
// 	INFOD("========= freePageHTML ======")
	_controle *ptrFct = premiereFct;
	void *temp = ptrFct;
	while(ptrFct != NULL) {
		temp = ptrFct->suiv;
		free(ptrFct);
		ptrFct = temp;
	};
}

//==========================================================
/* Crée les structures chainées "_controle" pour construire la page html */ /* 
	- libérer la mémoire lorsque les données ont été utilisées avec "freePageHTML" */
/*==========================================================*/
_controle *creerPageHTML(void)
{
	idx = 0;
	// NOTE modif test
// 	void *pStructCtrl = malloc(sizeof(void));
	void *pStructCtrl = malloc(sizeof(_controle));
	
	if(!pStructCtrl) {    // Si l'allocation a échoué.
		INFOE("l'allocation a échoué")
		return NULL;
	}

	CREER_FONCTION("fonction 1");
	void *premiere = pStructCtrl;	
	
	CREER_LABEL("Fonction 1", "Fonction 1", "labelTitre");
	CREER_BOUTON("btn1", 0, 1);
#if defined(__linux__) || OSX	
	CREER_LABEL_IMG("btn1", "./on.png", "label");
#else		// EPS32
	CREER_LABEL_IMG("btn1", "www/on.png", "label");
#endif
	CREER_P("p1", "info n°1", "infoP", 1);

	CREER_FONCTION("fonction 2");
	CREER_LABEL("fonction 2", "fonction 2", "labelTitre");
	CREER_BOUTON("btn2", 0, 1);
#if defined(__linux__) || OSX	
	CREER_LABEL_IMG("btn2", "./off.png", "label");
#else		// EPS32
	CREER_LABEL_IMG("btn2", "www/off.png", "label");
#endif
	CREER_P("p2", "info n°2", "infoP", 1);
	CREER_RANGE("rng1", 25);
	CREER_BR;
	CREER_COLOR("col1", "#FF0000");
	
	CREER_FONCTION("fonction 3");
	CREER_LABEL("fonction 3", "fonction 3", "labelTitre");
	CREER_RADIO("rad1", "Radio", "Radio1", 0);
	CREER_LABEL("rad1", "Radio1", "");
	CREER_BR;
	CREER_RADIO("rad2", "Radio", "Radio2", 1);
	CREER_LABEL("rad2", "Radio2", "");
	CREER_BR;
	CREER_RADIO("rad3", "Radio", "Radio3", 0);
	CREER_LABEL("rad3", "Radio3", "");

	CREER_FONCTION("fonction 4");
	CREER_LABEL("fonction 4", "fonction 4", "labelTitre");
	CREER_CHECKBOX("check1", "Checkbox", "Checkbox1", 1);
	CREER_LABEL("check1", "Checkbox1", "");
	CREER_BR;
	CREER_CHECKBOX("check2", "Checkbox", "Checkbox2", 1);
	CREER_LABEL("check2", "Checkbox2", "");
	CREER_BR;
	CREER_CHECKBOX("check3", "Checkbox", "Checkbox3", 0);
	CREER_LABEL("check3", "Checkbox3", "");
		
	CREER_FONCTION("fonction 6");
	CREER_LABEL("fonction 6", "Météo locale", "labelTitre");
	CREER_P("p3", "température extérieure", "infoP", 0);
	CREER_TEXTAREA("text1", "Temperature ext");
	CREER_P("p4", "température intérieure", "infoP", 0);
	CREER_TEXTAREA("text2", "Température et humidité");
	
// 	INFOD("========= structures objets pages HTML créées =========== ")
	return premiere;
}
