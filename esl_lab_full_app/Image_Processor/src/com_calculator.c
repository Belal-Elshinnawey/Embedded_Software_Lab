#include <stdio.h>
#include "green_mask.h"
#include "stdint.h"
#include "com_calculator.h"
#include "string.h"
#include "image_processor.h"
#include "rt_utils.h"
#include "spsc_queue.h"
#include "start_com_queue.h"
#include "profiling.h"
#include "udp_server.h"

SPSCQueue com_queue;

void calculate_com(uint8_t mask[HEIGHT][WIDTH], float *com_x, float *com_y)
{
    unsigned long sum_x = 0, sum_y = 0, count = 0;
    const unsigned long min_green_pixels = MIN_GREEN_PIXELS;

    for (int y = 0; y < HEIGHT; ++y)
    {
        for (int x = 0; x < WIDTH; ++x)
        {
            if (mask[y][x])
            {
                sum_x += x;
                sum_y += y;
                count++;
            }
        }
    }

    if (count >= min_green_pixels)
    {
        *com_x = (float)sum_x / (float)count;
        *com_y = (float)sum_y / (float)count;
    }
    else
    {
        *com_x = -1.0f;
        *com_y = -1.0f;
    }
}


void *com_calculator_thread(void *arg)
{ 
    set_realtime_priority(pthread_self(), 84);
    (void)arg; // unused
    uint8_t mask[HEIGHT][WIDTH];
    float com_x, com_y;
    TimeRecord thread_timing;
    UDPServerHandler  debug_server;
    udp_server_create(&debug_server, COM_THREAD_DEBUG_PORT);
    while (1)
    {
        clock_gettime(CLOCK_MONOTONIC, &(thread_timing.start_time));
        pthread_mutex_lock(&green_mask_queue_mutex);
        while (green_mask_queue_count == 0)
            pthread_cond_wait(&green_mask_queue_not_empty, &green_mask_queue_mutex);

        memcpy(mask, green_mask_queue[green_mask_queue_front], sizeof(mask));
        green_mask_queue_front = (green_mask_queue_front + 1) % GREEN_MASK_QUEUE_SIZE;
        green_mask_queue_count--;

        pthread_cond_signal(&green_mask_queue_not_full);
        pthread_mutex_unlock(&green_mask_queue_mutex);

        calculate_com(mask, &com_x, &com_y);

        COM value = { com_x, com_y };
        udp_server_send(&debug_server, (void *)&value, sizeof(value)); 

        if (!spsc_enqueue(&com_queue, value)) {
            COM discarded_com; //Might cause issues if the system is not timed correctly.
            spsc_dequeue(&com_queue, &discarded_com);
            spsc_enqueue(&com_queue, value);
        }

        if (stop_image_processing) {
            pthread_mutex_unlock(&green_mask_queue_mutex);
            clock_gettime(CLOCK_MONOTONIC, &(thread_timing.end_time));
            profile_thread(2, thread_timing);
            break;
        }
        clock_gettime(CLOCK_MONOTONIC, &(thread_timing.end_time));
        profile_thread(2, thread_timing);
    }

    return NULL;
}


int start_com_calculator_thread(pthread_t *thread)
{
    return pthread_create(thread, NULL, com_calculator_thread, NULL);
}