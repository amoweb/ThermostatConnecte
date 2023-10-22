/////////////////////////////////////////////////////////////////////////////
// OSX_RPI_ESP.h
/* pour permettre l'utilsation des fichiers sur les plateformes :
	- OSX
	- RPI
	_ ESP32
	*/
/////////////////////////////////////////////////////////////////////////////
#ifndef _OSX_RPI_ESP
#define _OSX_RPI_ESP

#if defined(__linux__) || OSX
#else		// EPS32
#endif

#if defined(__linux__) || OSX
	#include "utilitaire.h"
#else		// EPS32
	#include "esp_log.h"
	#define INFOE(format, ...);
	#define INFOA(format, ...);
	#define INFOD(format, ...);
	#define INFOL(format, ...);
	#define INFO(format, ...);
	
#endif

#endif	// _OSX_RPI_ESP