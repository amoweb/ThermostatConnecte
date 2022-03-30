#include <time.h>
#include <stdio.h>
#include "storage.h"
#include <stdbool.h>

static time_t timestamp_offset_seconds = 0;
static time_t initial_time_seconds = 0;

presence_s presence_array[7][2];

void init_presence_array() {
    for(unsigned int d=0; d<7; d++) {
        for(unsigned int i=0; i<2; i++) {
            presence_array[d][i].start_hour = 0;
            presence_array[d][i].start_minute = 0;
            presence_array[d][i].end_hour = 0;
            presence_array[d][i].end_minute = 0;
        }
    }
}

void print_presence_array()
{
    for(unsigned int d=0; d<7; d++) {
        for(unsigned int i=0; i<2; i++) {
            printf("Day %d %d/2 : %d:%d - %d:%d\n",
                    d, 
                    i,
                    presence_array[d][i].start_hour,
                    presence_array[d][i].start_minute,
                    presence_array[d][i].end_hour,
                    presence_array[d][i].end_minute
                    );
        }
    }
}

void set_presence_array_from_string(const char* data)
{
    char* p = (char*)&data[4];

    for(unsigned int d=0; d<7; d++) {
        for(unsigned int i=0; i<2; i++) {
            for(unsigned int c=0; c<4; c++) {

                long value = strtol(p, &p, 10);
                p = &p[5];

                switch(c) {
                    case 0: presence_array[d][i].start_hour = value; break;
                    case 1: presence_array[d][i].start_minute = value; break;
                    case 2: presence_array[d][i].end_hour = value; break;
                    case 3: presence_array[d][i].end_minute = value; break;
                }
            }
        }
    }
}

struct time get_current_time()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts); // time from boot

    time_t timestamp = ts.tv_sec - timestamp_offset_seconds + initial_time_seconds;

    struct time t;
    t.day = (timestamp % (7*24*60*60)) / (24 * 60 * 60);

    time_t rem = timestamp - (t.day * 24 * 60 * 60);

    t.hour = rem / (60 * 60);

    rem = rem - (t.hour * 60 * 60);

    t.minute = rem / 60;
    
    return t;
}

void set_current_time(struct time t)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts); // time from boot

    initial_time_seconds = (((t.day*24) + t.hour) * 60 + t.minute) * 60;
    timestamp_offset_seconds = ts.tv_sec;
}

// Limitation:  only works when comparing two times of the same week.
bool time_equals(struct time t1, struct time t2)
{
    return ( t1.day==t2.day && t1.hour==t2.hour && t1.minute==t2.minute );
}

struct time presence_get_next_start(struct time currentTime)
{
    unsigned int tcurrent = currentTime.hour*60 + currentTime.minute;

    // Limitation: only works if presence_array[day] is sorted.
    for(int i = 0; i < 2; i++) {
        presence_s p = presence_array[currentTime.day][i];
        unsigned int tpresence = p.start_hour*60 + p.start_minute;

        if(tpresence > tcurrent) {
            currentTime.hour = p.start_hour;
            currentTime.minute = p.start_minute;

            return currentTime;
        }
    }

    currentTime.hour = 99;
    return currentTime;
}

struct time presence_get_next_end(struct time currentTime) 
{
    unsigned int tcurrent = currentTime.hour*60 + currentTime.minute;

    // Limitation: only works if presence_array[day] is sorted.
    for(int i = 0; i < 2; i++) {
        presence_s p = presence_array[currentTime.day][i];
        unsigned int tpresence = p.end_hour*60 + p.end_hour;

        if(tpresence > tcurrent) {
            currentTime.hour = p.end_hour;
            currentTime.minute = p.end_hour;

            return currentTime;
        }
    }

    currentTime.hour = 99;
    return currentTime;
}

