#include "estimator.h"

#include <stdio.h>

struct estimator {
    double slopeHistory[2];
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
static double slope_default = 2;

void estimator_init()
{
    p_.slopeHistory[0] = -1;
    p_.slopeHistory[1] = -1;
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

            p_.slopeHistoryIndex = (p_.slopeHistoryIndex + 1) % 2;
        }
    }

    p_.pre_heat = heat;
}

double estimator_get_slope()
{
    double s = 0;

    if(p_.slopeHistory[0] == -1 && p_.slopeHistory[1] == -1) {
        s = slope_default;
    } else if(p_.slopeHistory[0] == -1) {
        s = p_.slopeHistory[1];
    } else if(p_.slopeHistory[1] == -1) {
        s = p_.slopeHistory[0];
    } else {
        s = (p_.slopeHistory[1] + p_.slopeHistory[0]) / 2;
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
    if(slope != 3) {
        printf("FAIL 2\n");
        while(true)
            ;
    }
    

    printf("Test OK\n");
}

