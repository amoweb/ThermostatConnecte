#ifndef HEURE_SNTP_H
#define HEURE_SNTP_H

#include <time.h>

void maj_heure(void);
void start_SNTP(void);
void stop_SNTP(void);

void init_SNTP(void);
void sync_SNTP(void);
struct tm maj_Heure(void);

#endif // HEURE_SNTP_H
