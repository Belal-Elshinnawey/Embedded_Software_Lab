#define _GNU_SOURCE
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>     
#include <stdlib.h>
#include <signal.h>
#include "rt_utils.h"


void pin_thread_to_core(pthread_t thread, int core_id)
{
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);

    int result = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
    if (result != 0)
    {
        fprintf(stderr, "Failed to pin thread to core %d (error %d)\n", core_id, result);
    }
    else
    {
        printf("Thread pinned to core %d successfully.\n", core_id);
    }
}

void set_realtime_priority(pthread_t thread, int priority) {
    struct sched_param p = { .sched_priority = priority };
    int ret = pthread_setschedparam(thread, SCHED_FIFO, &p);
    if (ret != 0) {
        errno = ret;
        perror("pthread_setschedparam");
    }
}

double timespec_diff_us(const struct timespec *start, const struct timespec *end) {
    double sec  = (double)(end->tv_sec  - start->tv_sec);
    double nsec = (double)(end->tv_nsec - start->tv_nsec);
    return sec * 1e6 + nsec * 1e-3;
}
