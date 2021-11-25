#include "hysteresis.h"

struct hystersis {
    bool heat;
    double target_temperature;
    double low_threshold;
    double high_threshold;
};

static struct hystersis h_;

void hysteresis_init()
{
    h_.heat = false;
    h_.target_temperature = 17;
    h_.low_threshold = 0.25;
    h_.high_threshold = 0.25;
}

void hysteresis_set_target(double target_temperature)
{
    h_.target_temperature = target_temperature;
}

void hysteresis_set_thresholds(double low_threshold, double high_threshold)
{
    h_.low_threshold = low_threshold;
    h_.high_threshold = high_threshold;
}

void hysteresis_reset()
{
    h_.heat = false;
}

void hysteresis_step(double temperature, bool* heat)
{
    if(h_.heat) {
        h_.heat = (temperature < h_.target_temperature + h_.high_threshold);
    } else {
        h_.heat = (temperature < h_.target_temperature - h_.low_threshold);
    }
        
    *heat = h_.heat;
}

