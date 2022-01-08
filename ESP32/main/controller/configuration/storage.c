#include <time.h>

static time_t timestamp_offset = 0;
static time_t initial_time = 0;

void get_current_time(unsigned int* hour, unsigned int* minute, unsigned int* day)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts); // time from boot

    time_t timestamp = ts.tv_sec - timestamp_offset + initial_time;

    *day = (timestamp % (7*24*60*60)) / (24 * 60 * 60);

    time_t rem = timestamp - (*day * 24 * 60 * 60);

    *hour = rem / (60 * 60);

    rem = rem - (*hour * 60 * 60);

    *minute = rem / 60;
}

void set_current_time(unsigned int hour, unsigned int minute, unsigned int day)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts); // time from boot

    initial_time = (((day*24) + hour) * 60 + minute) * 60;
    timestamp_offset = ts.tv_sec;
}

