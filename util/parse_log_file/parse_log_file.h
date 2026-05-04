#ifndef PARSE_LOG_FILE
#define PARSE_LOG_FILE
#include <stdio.h>

    typedef struct {
        long start;
        long end;
    }time_range;
    typedef enum {
        STATE_NONE = 0,
        STATE_POWER_USAGE = 1
    } parse_state_t;
    typedef struct {
        FILE *dest;
        long start;
        long end;
        int in_power_usage;
        int expect_timestamp;
        int expect_value;

        long current_timestamp;
    } parse_power_usage_ctx;
    void sleep_us(long microseconds);
    long get_time_us(void);
    int parse_stats_targeted_power(time_range *t_range,const char *log_filename);
#endif