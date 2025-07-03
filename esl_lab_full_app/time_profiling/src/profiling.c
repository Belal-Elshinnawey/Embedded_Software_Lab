#include "profiling.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static TimeRecord green_mask_data[MAX_ENTRIES];
static TimeRecord com_calculator_data[MAX_ENTRIES];
static TimeRecord start_data[MAX_ENTRIES];

static int green_mask_count = 0;
static int com_calculator_count = 0;
static int start_count = 0;

static int green_mask_written = 0;
static int com_calculator_written = 0;
static int start_written = 0;

static double timespec_to_ms(const struct timespec* t) {
    return t->tv_sec * 1000.0 + t->tv_nsec / 1e6;
}

static void write_csv(const char* filename, TimeRecord* data_array) {
    FILE* f = fopen(filename, "w");
    if (!f) {
        perror("Failed to open CSV file");
        return;
    }

    fprintf(f, "index,start_ms,end_ms,duration_ms\n");
    for (int i = 0; i < MAX_ENTRIES; i++) {
        double start_ms = timespec_to_ms(&data_array[i].start_time);
        double end_ms = timespec_to_ms(&data_array[i].end_time);
        double duration = end_ms - start_ms;
        fprintf(f, "%d,%.3f,%.3f,%.3f\n", i, start_ms, end_ms, duration);
    }

    fclose(f);
}

void store_time(int thread_id, TimeRecord record) {
    if (thread_id == 1) {
        if (green_mask_count < MAX_ENTRIES) {
            green_mask_data[green_mask_count++] = record;
            if (green_mask_count == MAX_ENTRIES && !green_mask_written) {
                write_csv("green_mask.csv", green_mask_data);
                green_mask_written = 1;
            }
        }
    } else if (thread_id == 2) {
        if (com_calculator_count < MAX_ENTRIES) {
            com_calculator_data[com_calculator_count++] = record;
            if (com_calculator_count == MAX_ENTRIES && !com_calculator_written) {
                write_csv("com_calculator.csv", com_calculator_data);
                com_calculator_written = 1;
            }
        }
    } else if (thread_id == 3) {
        if (start_count < MAX_ENTRIES) {
            start_data[start_count++] = record;
            if (start_count == MAX_ENTRIES && !start_written) {
                write_csv("start.csv", start_data);
                start_written = 1;
            }
        }
    }
}

void profile_thread(int thread_id, TimeRecord record) {
#ifdef MEASURE_THREADS
    store_time(thread_id, record);
#endif
}