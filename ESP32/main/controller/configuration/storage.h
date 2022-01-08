#pragma once

/**
  * Get the current time (hour, minute, day)
  * @param[out] hour in [0, 23]
  * @param[out] minute in [0, 60]
  * @param[out] day in [0, 6] for Monday - Sunday
  */
void get_current_time(unsigned int* hour, unsigned int* minute, unsigned int* day);

/**
  * Set the current time (hour, minute, day)
  * @param[in] hour in [0, 23]
  * @param[in] minute in [0, 60]
  * @param[in] day in [0, 6] for Monday - Sunday
  */
void set_current_time(unsigned int hour, unsigned int minute, unsigned int day);

