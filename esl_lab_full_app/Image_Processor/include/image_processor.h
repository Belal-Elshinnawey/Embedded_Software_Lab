#ifndef IMAGE_PROCESSOR_H
#define IMAGE_PROCESSOR_H

#include <stdint.h>
#include <pthread.h>
#include <signal.h>

extern volatile sig_atomic_t stop_image_processing;
int start_image_processor();
void stop_image_processor();

#endif
