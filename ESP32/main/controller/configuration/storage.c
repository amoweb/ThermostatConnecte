#include <time.h>
#include <stdio.h>
#include "storage.h"
#include <stdbool.h>

static time_t timestamp_offset_seconds = 0;
static time_t initial_time_seconds = 0;
static double target_temp_presence = 19;
static double target_temp_absence = 17;

void set_temperature_target(double presence, double absence)
{
    target_temp_presence = presence;
    target_temp_absence = absence;
}

void get_temperature_target(double* presence, double* absence)
{
    *presence = target_temp_presence;
    *absence = target_temp_absence;
}


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
    printf("get_current_time\n");
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
    printf("set_current_time\n");
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

unsigned int time_duration_minute(struct time t1, struct time t2)
{
    /*
       Exemple:
       t1_minutes = ((4*24) + 7)*60 + 0 = 6180 = Jeudi à 7h00
       t2_minutes = ((2*24) + 7)*60 + 0 = 3300 = Mardi à 7h00
       diff_minutes = (3300 + 10080) - 6180 = 7200
       diff_jours = 7200/(24*60) = 5 jours
       OK
     */

    unsigned int t1_minutes = ((t1.day*24) + t1.hour)*60 + t1.minute;
    unsigned int t2_minutes = ((t2.day*24) + t2.hour)*60 + t2.minute;

    unsigned int diff_minutes;

    // Easy case
    if(t1_minutes < t2_minutes) {
        diff_minutes = t2_minutes - t1_minutes;
    } else if(t1_minutes == t2_minutes) {
        diff_minutes = 0;
    } else {
        diff_minutes = (t2_minutes + 7*24*60) - t1_minutes;
    }

    return diff_minutes;
}

struct time time_duration(struct time t1, struct time t2)
{
    unsigned int diff_minutes = time_duration_minute(t1, t2);

    struct time diff;

    diff.day = diff_minutes / (24*60);
    diff_minutes = diff_minutes - 24*60*diff.day;
    diff.hour = diff_minutes / 60;
    diff_minutes = diff_minutes - 60*diff.hour;
    diff.minute = diff_minutes;

    return diff;
}

void time_test()
{
    struct time t1;
    struct time t2;
    struct time diff;

    // Test 1
    t1.day = 1; t1.hour = 1; t1.minute = 1;
    t2.day = 2; t2.hour = 2; t2.minute = 2;
    diff = time_duration(t1, t2);

    if(diff.day != 1 || diff.hour != 1 || diff.minute != 1) {
        printf("FAIL 1\n");
        while(true)
            ;
    }

    // Test 2
    t1.day = 1; t1.hour = 1; t1.minute = 1;
    t2.day = 2; t2.hour = 2; t2.minute = 3;
    diff = time_duration(t1, t2);

    if(diff.day != 1 || diff.hour != 1 || diff.minute != 2) {
        printf("FAIL 2\n");
        while(true)
            ;
    }

    printf("Tests OK\n");
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


void test_time()
{
    struct time t1;
    t1.hour = 3;
    t1.minute = 4;
    t1.day = 5;
    set_current_time(t1);

    struct time t2 = get_current_time();

    if(t1.hour != t2.hour || t1.minute != t2.minute || t1.day != t2.day) {
        printf("test_time: ERROR\n");
        printf("t1 = %2d:%2d day=%d\n", t1.hour, t1.minute, t1.day);
        printf("t2 = %2d:%2d day=%d\n", t2.hour, t2.minute, t2.day);
        while(true)
            ;
    }

    printf("test_time OK\n");
}
