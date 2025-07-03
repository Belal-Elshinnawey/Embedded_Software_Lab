#ifndef GREEN_MASK_THREAD_H
#define GREEN_MASK_THREAD_H

#include <stdint.h>
#include <pthread.h>

// #define WIDTH 1280
// #define HEIGHT 960

// #define WIDTH 640
// #define HEIGHT 360

// #define GREEN_MASK_QUEUE_SIZE 5
// // HSV green range thresholds
// #define GREEN_H_MIN 95.0
// #define GREEN_H_MAX 180.0
// #define GREEN_S_MIN 0.25
// #define GREEN_V_MIN 0.4

extern uint8_t green_mask_queue[GREEN_MASK_QUEUE_SIZE][HEIGHT][WIDTH];
extern int green_mask_queue_front, green_mask_queue_rear, green_mask_queue_count;
extern pthread_mutex_t green_mask_queue_mutex;
extern pthread_cond_t green_mask_queue_not_empty;
extern pthread_cond_t green_mask_queue_not_full;

int start_green_mask_thread(pthread_t *thread);

#endif
