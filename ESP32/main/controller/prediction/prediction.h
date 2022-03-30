#ifndef PREDICTION_H
#define PREDICTION_H

#include <stdbool.h>
#include "../configuration/storage.h"

/*
   Le but est de calculer le temps de chauffe pour que la pièce soit chaude à
   une heure préciste.
   Par exemple, si le module calcul un temps de chauffe de 2 degré par heure
   (slope).  S'il faut qu'il fasse 19 degrés à 8h et qu'il fait 17 degrés. Le
   chauffage doit se déclancher à 7h (delta_time_hour = 2).
 */

void prediction_init();

void prediction_step(double temperature, bool heat, struct time currentTime);

#endif

