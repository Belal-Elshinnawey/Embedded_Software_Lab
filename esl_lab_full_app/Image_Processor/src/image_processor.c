#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <sched.h>      
#include <unistd.h>     
#include "green_mask.h"
#include "com_calculator.h"
#include <stdlib.h>
#include "image_processor.h"
#include <signal.h>
#include "rt_utils.h"

volatile sig_atomic_t stop_image_processing = 0;
static pthread_t producer_thread;
static pthread_t consumer_thread;

int start_image_processor()
{
    if (start_green_mask_thread(&producer_thread) != 0)
    {
        fprintf(stderr, "Failed to start green mask producer thread.\n");
        return 1;
    }
#ifdef PLATFORM_RPI
    pin_thread_to_core(producer_thread, 1);
#elif defined(PLATFORM_DE10)
    pin_thread_to_core(producer_thread, 0);
#endif
    if (start_com_calculator_thread(&consumer_thread) != 0)
    {
        fprintf(stderr, "Failed to start COM calculator thread.\n");
        return 1;
    }
#ifdef PLATFORM_RPI
    pin_thread_to_core(consumer_thread, 2);
#elif defined(PLATFORM_DE10)
    pin_thread_to_core(consumer_thread, 1);
#endif
    return 0;
}

void stop_image_processor()
{
    stop_image_processing = 1;
    pthread_cond_broadcast(&green_mask_queue_not_empty);
    pthread_cond_broadcast(&green_mask_queue_not_full);
    pthread_join(producer_thread, NULL);
    pthread_join(consumer_thread, NULL);
    printf("Threads terminated.\n");
}
