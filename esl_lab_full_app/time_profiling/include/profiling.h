#ifndef PROFILING_H
#define PROFILING_H

#include <time.h>

#define MAX_ENTRIES 10000

typedef struct {
    struct timespec start_time;
    struct timespec end_time;
} TimeRecord;

void store_time(int thread_id, TimeRecord record);
void profile_thread(int thread_id, TimeRecord record);
#endif // PROFILING_H
