#include "hysteresis.h"

struct prediction {
};

static struct prediction p_;

static int min_temperature_delta = 1;
static int max_time_delta_hour = 3;

void prediction_init()
{
}

void prediction_step(double temperature, bool heat)
{
}

