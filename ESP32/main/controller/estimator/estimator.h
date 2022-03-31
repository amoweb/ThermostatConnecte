#ifndef ESTIMATOR_H
#define ESTIMATOR_H

#include <stdbool.h>
#include "../configuration/storage.h"

/*
   Le but est de calculer le temps de chauffe pour que la pièce soit chaude à
   une heure préciste.

   Par exemple, si le module calcul un temps de chauffe de 2 degré par heure
   (slope).  S'il faut qu'il fasse 19 degrés à 8h et qu'il fait 17 degrés. Le
   chauffage doit se déclancher à 7h (delta_time_hour = 2).

    Principe :
    - Considère la chauffe linéaire.
    - Enregistre les données uniquement lorsque la chauffe est en cours (heat==1)
    - Enregistre un premier point (température1, temps1) lorsque la chauffe démarre (front montant heat)
    - Enregistre le dernier point (température2, temps2) lorsque la chauffe s'arrête (front descendant heat)
    - Calcul la pente uniquement si la (temperature2 - temperature1) > 1°C
    - Le résultat est la moyenne des deux dernières estimations.
 */

void estimator_init();

/**
  * Met à jour l'estimation de pente de chaffe.
  * @param[in] currentTemperature double
  * @param[in] heat vrai si on chauffe
  * @param[in] currentTime struct time
  **/
void estimator_step(double currentTemperature, bool heat, struct time currentTime);

/**
  * Renvoie le nombre de degré par heure estimé.
  */
double estimator_get_slope();

void estimator_test();

#endif

