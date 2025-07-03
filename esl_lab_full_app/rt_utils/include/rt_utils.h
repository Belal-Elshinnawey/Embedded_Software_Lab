#ifndef RT_UTILS_H
#define RT_UTILS_H

#include <stdint.h>
#include <pthread.h>
#include "time.h"

typedef struct {
    struct timespec fired;      
    struct timespec finished;   
    double exec_time_us;  
} CycleTimestamp;


void set_realtime_priority(pthread_t thread, int priority);
double timespec_diff_us(const struct timespec *start, const struct timespec *end);
void pin_thread_to_core(pthread_t thread, int core_id);

#endif
