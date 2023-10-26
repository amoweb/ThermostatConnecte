#ifndef HYSTERESIS_H
#define HYSTERESIS_H

#include <stdbool.h>

void hysteresis_init();

/*
 * Define the target temperature.
 * @param[in] target_temperature °C
 */
void hysteresis_set_target(double target_temperature);

/*
 * Get the target temperature.
 * @return target_temperature °C
 */
double hysteresis_get_target();

/*
 * Define the low and high threshold. Low hystersis threshold is
 * target_temperature - low_threshold. High threshold is target_temperature +
 * high_threshold.
 * 
 * @param[in] low_threshold
 * @param[in] high_threshold
 */
void hysteresis_set_thresholds(double low_threshold, double high_threshold);

/*
 * Reset the controller internal memory.
 */
void hysteresis_reset();

/*
 * Perform on step.
 * @param[in] temperature
 * @param[out] heat
 */
void hysteresis_step(double temperature, bool* heat);

#endif

