#include "estimator.h"

#include <stdio.h>

#define SLOPE_HISTORY_LENGTH 5

struct estimator {
    double slopeHistory[SLOPE_HISTORY_LENGTH];
    unsigned int slopeHistoryIndex;

    double temperature1;
    struct time time1;

    double temperature2;
    struct time time2;

    bool pre_heat;
};

static struct estimator p_;

static int min_temperature_delta = 1;
static double slope_min = 0.3;
static double slope_max = 4;

void estimator_init()
{
    for(int i = 0; i < SLOPE_HISTORY_LENGTH; i++) {
        p_.slopeHistory[i] = slope_max;
    }

    p_.slopeHistoryIndex = 0;

    p_.time1.day = 0;
    p_.time1.hour = 0;
    p_.time1.minute = 0;
    p_.time2.day = 0;
    p_.time2.hour = 0;
    p_.time2.minute = 0;
    p_.pre_heat = false;
}

void estimator_step(double temperature, bool heat, struct time currentTime)
{
    // Heat start
    if(heat && !p_.pre_heat) {
        p_.time1 = currentTime;
        p_.temperature1 = temperature;

        printf("Estimation t1: %2d:%2d day=%d : %2.f degrees\n", p_.time1.hour, p_.time1.minute, p_.time1.day, p_.temperature1);
    }

    if(!heat && p_.pre_heat) {
        p_.time2 = currentTime;
        p_.temperature2 = temperature;

        printf("Estimation t1: %2d:%2d day=%d : %2.f degrees\n", p_.time1.hour, p_.time1.minute, p_.time1.day, p_.temperature1);
        printf("Estimation t2: %2d:%2d day=%d : %2.f degrees\n", p_.time2.hour, p_.time2.minute, p_.time2.day, p_.temperature2);

        unsigned int delta_minute = time_duration_minute(p_.time1, p_.time2);

        double delta_temperature = p_.temperature2 - p_.temperature1;

        if(delta_temperature >= min_temperature_delta) {
            double slope = delta_temperature / (delta_minute / 60.0);
            printf("Estimator: %.2f degree/hour\n", slope);
    
            p_.slopeHistory[p_.slopeHistoryIndex] = slope;

            p_.slopeHistoryIndex = (p_.slopeHistoryIndex + 1) % SLOPE_HISTORY_LENGTH;
        }
    }

    p_.pre_heat = heat;
}

double estimator_get_slope()
{
    double s = slope_max;

    // Compute sliding min over history
    for(int i = 0; i < SLOPE_HISTORY_LENGTH; i++) {
        if(p_.slopeHistory[i] < s) {
            s = p_.slopeHistory[i];
        }
    }

    // Clipping
    if(s < slope_min) {
        return slope_min;
    } else if(s > slope_max) {
        return slope_max;
    } else {
        return s;
    }
}

void estimator_test()
{
    estimator_init();

    struct time t;
    double slope;

    t.day = 0; t.hour = 1; t.minute = 0;
    estimator_step(17, true, t);

    t.day = 0; t.hour = 2; t.minute = 0;
    estimator_step(19, false, t);

    slope = estimator_get_slope();
    printf("slope 1 = %f\n", slope);
    if(estimator_get_slope() != 2) {
        printf("FAIL 1\n");
        while(true)
            ;
    }

    t.day = 0; t.hour = 4; t.minute = 0;
    estimator_step(18, true, t);

    t.day = 0; t.hour = 4; t.minute = 30;
    estimator_step(20, false, t);
    
    slope = estimator_get_slope();
    printf("slope 2 = %f\n", slope);
    if(slope != 2) {
        printf("FAIL 2\n");
        while(true)
            ;
    }
    

    printf("Test OK\n");
}

