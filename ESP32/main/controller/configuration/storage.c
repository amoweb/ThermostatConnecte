#include <time.h>
#include <stdio.h>
#include "storage.h"

static time_t timestamp_offset = 0;
static time_t initial_time = 0;

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

void print_presence_array() {
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

void set_presence_array_from_string(const char* data) {
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

