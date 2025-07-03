#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include "green_mask.h"
#include <pthread.h>
#include <time.h>
#include "image_processor.h"
#include "rt_utils.h"
#include "profiling.h"
#include "udp_server.h"


#define MAX_LABELS 10000 

typedef struct {
    int x, y;
} Point;

uint8_t green_mask_queue[GREEN_MASK_QUEUE_SIZE][HEIGHT][WIDTH];
int green_mask_queue_front, green_mask_queue_rear, green_mask_queue_count;
pthread_mutex_t green_mask_queue_mutex;
pthread_cond_t green_mask_queue_not_empty;
pthread_cond_t green_mask_queue_not_full;

void erode_mask(uint8_t mask[HEIGHT][WIDTH], uint8_t temp[HEIGHT][WIDTH]) {
    int x, y, dx, dy;
    for (y = 1; y < HEIGHT-1; y++) {
        for (x = 1; x < WIDTH-1; x++) {
            uint8_t keep = 1;
            for (dy = -1; dy <= 1; dy++) {
                for (dx = -1; dx <= 1; dx++) {
                    if (mask[y+dy][x+dx] == 0) {
                        keep = 0;
                        goto done_check;
                    }
                }
            }
            done_check:
            temp[y][x] = keep;
        }
    }
    for (x = 0; x < WIDTH; x++) { temp[0][x] = 0; temp[HEIGHT-1][x] = 0; }
    for (y = 0; y < HEIGHT; y++) { temp[y][0] = 0; temp[y][WIDTH-1] = 0; }
}

void dilate_mask(uint8_t mask[HEIGHT][WIDTH], uint8_t temp[HEIGHT][WIDTH]) {
    int x, y, dx, dy;
    for (y = 1; y < HEIGHT-1; y++) {
        for (x = 1; x < WIDTH-1; x++) {
            uint8_t set = 0;
            for (dy = -1; dy <= 1; dy++) {
                for (dx = -1; dx <= 1; dx++) {
                    if (mask[y+dy][x+dx] != 0) {
                        set = 1;
                        goto done_check;
                    }
                }
            }
            done_check:
            temp[y][x] = set;
        }
    }
    for (x = 0; x < WIDTH; x++) { temp[0][x] = 0; temp[HEIGHT-1][x] = 0; }
    for (y = 0; y < HEIGHT; y++) { temp[y][0] = 0; temp[y][WIDTH-1] = 0; }
}

void filter_small_blobs(uint8_t mask[HEIGHT][WIDTH], int min_size) {
    int label_map[HEIGHT][WIDTH];
    int label_sizes[MAX_LABELS] = {0};
    int current_label = 1;

    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            label_map[y][x] = 0;
        }
    }

    Point queue[HEIGHT * WIDTH];
    int queue_start, queue_end;

    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (mask[y][x] == 0 || label_map[y][x] != 0)
                continue;
            queue_start = 0;
            queue_end = 0;
            queue[queue_end++] = (Point){x, y};
            label_map[y][x] = current_label;
            int size = 1;
            while (queue_start < queue_end) {
                Point p = queue[queue_start++];
                int px = p.x;
                int py = p.y;
                int neighbors[4][2] = {{px-1, py}, {px+1, py}, {px, py-1}, {px, py+1}};
                for (int i = 0; i < 4; i++) {
                    int nx = neighbors[i][0];
                    int ny = neighbors[i][1];
                    if (nx >= 0 && nx < WIDTH && ny >= 0 && ny < HEIGHT) {
                        if (mask[ny][nx] != 0 && label_map[ny][nx] == 0) {
                            label_map[ny][nx] = current_label;
                            queue[queue_end++] = (Point){nx, ny};
                            size++;
                        }
                    }
                }
            }

            label_sizes[current_label] = size;
            current_label++;
            if (current_label >= MAX_LABELS) {
                fprintf(stderr, "Warning: reached max blob count %d\n", MAX_LABELS);
                return;
            }
        }
    }
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            int lbl = label_map[y][x];
            if (lbl != 0 && label_sizes[lbl] < min_size) {
                mask[y][x] = 0;
            }
        }
    }
}

void rgb_to_hsv(uint8_t r, uint8_t g, uint8_t b, float *h, float *s, float *v)
{
    float fr = r / 255.0f;
    float fg = g / 255.0f;
    float fb = b / 255.0f;
    float max = fmaxf(fmaxf(fr, fg), fb);
    float min = fminf(fminf(fr, fg), fb);
    float delta = max - min;
    *v = max;
    *s = (max == 0) ? 0 : delta / max;

    if (delta == 0)
        *h = 0;
    else if (max == fr){
        // *h = 60.0f * fmodf(((fg - fb) / delta), 6.0f);
        float temp_h = ((fg - fb) / delta);
        if (temp_h < 0) temp_h += 6.0f;
        *h = 60.0f * temp_h;
    } else if (max == fg)
        *h = 60.0f * (((fb - fr) / delta) + 2.0f);
    else
        *h = 60.0f * (((fr - fg) / delta) + 4.0f);

    if (*h < 0)
        *h += 360.0f;
}

void *green_mask_thread(void *arg)
{
    set_realtime_priority(pthread_self(), 85);
    GstElement *pipeline, *source, *capsfilter, *decoder, *convert, *appsink;
    GstCaps *source_caps, *sink_caps;
    GstStateChangeReturn ret;
    TimeRecord thread_timing;
    uint8_t green_mask[HEIGHT][WIDTH];

    gst_init(NULL, NULL);

#ifdef PLATFORM_RPI
    source = gst_element_factory_make("v4l2src", "source");
    capsfilter = gst_element_factory_make("capsfilter", "capsfilter");
    decoder = gst_element_factory_make("v4l2jpegdec", "decoder");
    appsink = gst_element_factory_make("appsink", "appsink");
    convert = gst_element_factory_make("v4l2convert", "convert");
#elif defined(PLATFORM_DE10)
    source = gst_element_factory_make("v4l2src", "source");
    capsfilter = gst_element_factory_make("capsfilter", "capsfilter");
    decoder = gst_element_factory_make("jpegdec", "decoder");
    appsink = gst_element_factory_make("appsink", "appsink");
    convert = gst_element_factory_make("videoconvert", "convert");
#endif    




    if (!source || !capsfilter || !decoder || !convert || !appsink) {
        fprintf(stderr, "Failed to create GStreamer elements.\n");
        return NULL;
    }

    g_object_set(source, "device", VIDEO_DEVICE, NULL);

    source_caps = gst_caps_new_simple("image/jpeg",
                                      "width", G_TYPE_INT, WIDTH,
                                      "height", G_TYPE_INT, HEIGHT,
                                      "framerate", GST_TYPE_FRACTION, FRAME_RATE, 1,
                                      NULL);
    g_object_set(capsfilter, "caps", source_caps, NULL);
    gst_caps_unref(source_caps);

    sink_caps = gst_caps_new_simple("video/x-raw",
                                    "format", G_TYPE_STRING, "RGB",
                                    NULL);
    g_object_set(appsink,
                 "caps", sink_caps,
                 "emit-signals", FALSE,
                 "sync", FALSE,
                 "drop", TRUE,
                 "max-buffers", 1,
                 NULL);
    gst_caps_unref(sink_caps);

    pipeline = gst_pipeline_new("rgb-pipeline");
    gst_bin_add_many(GST_BIN(pipeline), source, capsfilter, decoder, convert, appsink, NULL);

    if (!gst_element_link_many(source, capsfilter, decoder, convert, appsink, NULL)) {
        fprintf(stderr, "Failed to link elements.\n");
        gst_object_unref(pipeline);
        return NULL;
    }

    ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE)
    {
        fprintf(stderr, "Failed to start pipeline.\n");
        gst_object_unref(pipeline);
        return NULL;
    }

    float h, s, v; 
    uint8_t r, g, b;
    int idx, x, y;
    GstSample *sample;
    GstMapInfo map;
    GstBuffer *buffer;
    guint8 *data;
    UDPServerHandler  debug_server;
    uint8_t temp_mask[HEIGHT][WIDTH];
    udp_server_create(&debug_server, GREEN_MASK_DEBUG_PORT);
    while (1){

        clock_gettime(CLOCK_MONOTONIC, &(thread_timing.start_time));

        sample = gst_app_sink_try_pull_sample(GST_APP_SINK(appsink), 2 * GST_SECOND);
        if (!sample)
        {
            printf("[GREEN MASK] No sample\n");
            clock_gettime(CLOCK_MONOTONIC, &(thread_timing.end_time));
            profile_thread(1, thread_timing);
            continue;
        }
        buffer = gst_sample_get_buffer(sample);

        if (gst_buffer_map(buffer, &map, GST_MAP_READ))
        {
            data = map.data;
            for (y = 0; y < HEIGHT; ++y)
            {
                for (x = 0; x < WIDTH; ++x)
                {
                    idx = (y * WIDTH + x) * 3;
                    r = data[idx];
                    g = data[idx + 1];
                    b = data[idx + 2];

                    rgb_to_hsv(r, g, b, &h, &s, &v);

                    green_mask[y][x] = (h >= GREEN_H_MIN && h <= GREEN_H_MAX &&
                                        s >= GREEN_S_MIN && v >= GREEN_V_MIN) ? 1 : 0;
                }
            }
            erode_mask(green_mask, temp_mask);
            dilate_mask(temp_mask, green_mask);
            filter_small_blobs(green_mask, 600);
            udp_server_send_large(&debug_server, green_mask, sizeof(green_mask));
            gst_buffer_unmap(buffer, &map);
        }
        gst_sample_unref(sample);

        pthread_mutex_lock(&green_mask_queue_mutex);
        while (green_mask_queue_count == GREEN_MASK_QUEUE_SIZE)
            pthread_cond_wait(&green_mask_queue_not_full, &green_mask_queue_mutex);

        memcpy(green_mask_queue[green_mask_queue_rear], green_mask, sizeof(green_mask));
        green_mask_queue_rear = (green_mask_queue_rear + 1) % GREEN_MASK_QUEUE_SIZE;
        green_mask_queue_count++;

        pthread_cond_signal(&green_mask_queue_not_empty);
        pthread_mutex_unlock(&green_mask_queue_mutex);
        clock_gettime(CLOCK_MONOTONIC, &(thread_timing.end_time));
        profile_thread(1, thread_timing);
 
    }

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    return NULL;
}


int start_green_mask_thread(pthread_t *thread)
{
    return pthread_create(thread, NULL, green_mask_thread, NULL);
}

