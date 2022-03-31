#pragma once

#include <stdbool.h>

struct time {
    unsigned int hour;   // hour in [0, 23]
    unsigned int minute; // minute in [0, 60]
    unsigned int day;    // day in [0, 6] for Monday - Sunday
};

/**
  * Get the current time (hour, minute, day)
  * @return struct time
  */
struct time get_current_time();

/**
  * Set the current time (hour, minute, day)
  * @param[in] struct time
  */
void set_current_time(struct time t);

/**
  * Returns true when time1 == time 2.
  * Limitation: times (day, h, m, s) must be of the same week
  * return bool
  */
bool time_equals(struct time t1, struct time t2);

/**
  * Computation duration from t1 to t2.
  * Returns: (t2 - t1)
  * Limitation: difference can't be more than 7 days
  **/
struct time time_duration(struct time t1, struct time t2);

/**
  * Computation duration from t1 to t2.
  * Returns: (t2 - t1)
  * Limitation: difference can't be more than 7 days
  **/
unsigned int time_duration_minute(struct time t1, struct time t2);

void time_test();

void init_presence_array();
void print_presence_array();
void set_presence_array_from_string(const char* data);

typedef struct presence {
    unsigned int start_hour;
    unsigned int start_minute;
    unsigned int end_hour;
    unsigned int end_minute;
} presence_s;

/**
 Get the next presence start
 @param[in] currentTime
 @return time (hour is 99 if nothing is found)
 **/
struct time presence_get_next_start(struct time currentTime);

/**
 Get the next presence end
 @param[in] currentTime
 @return time (hour is 99 if nothing is found)
 **/
struct time presence_get_next_end(struct time currentTime);

presence_s presence_array[7][2];

