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
    //h-1 and w-1 to avoid out of bound neighbors.
    for (y = 1; y < HEIGHT-1; y++) {
        for (x = 1; x < WIDTH-1; x++) {
            uint8_t keep = 1; // if keep is 1, the pixel remains white, 0 means black.
            for (dy = -1; dy <= 1; dy++) {
                for (dx = -1; dx <= 1; dx++) {
                    if (mask[y+dy][x+dx] == 0) {// if the outer neighbors are black (left by 1, up by 1 or right by 1, down by 1) 
                        keep = 0; //then this is black too.
                        goto done_check; // if set to black, exit.
                        // goto is not a big issue because we don't use it to exit the scope of the functions.
                        // also cleaner than multiple breaks.
                    }
                }
            }
            done_check://goto and loop end both come here
            // write output to a different buffer so the image does not become all 0
            temp[y][x] = keep; 
        }
    }
    // set the edge pixels to black since they dont have neighbors.
    for (x = 0; x < WIDTH; x++) { temp[0][x] = 0; temp[HEIGHT-1][x] = 0; } // top and bottom edges
    for (y = 0; y < HEIGHT; y++) { temp[y][0] = 0; temp[y][WIDTH-1] = 0; } // left right edges.
}

void dilate_mask(uint8_t mask[HEIGHT][WIDTH], uint8_t temp[HEIGHT][WIDTH]) {
    int x, y, dx, dy;
    // same story as erode, but this time, if any are 1, then make the pixel one
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
    int label_map[HEIGHT][WIDTH]; // to hold the labled blobs.
    int label_sizes[MAX_LABELS] = {0}; // the size of each blob
    int current_label = 1;

    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            label_map[y][x] = 0; //can be done with memset
        }
    }

    Point queue[HEIGHT * WIDTH]; // queue to hold the xy coord of the pixels.
    int queue_start, queue_end; // start and end of a blob

    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (mask[y][x] == 0 || label_map[y][x] != 0)// if already labled or not a white pixel, skip.
                continue;
            // this pixel is not labled and white
            queue_start = 0;
            queue_end = 0;
            queue[queue_end++] = (Point){x, y};// record the pixel index in the queue
            label_map[y][x] = current_label; // give the pixel a label
            int size = 1;
            while (queue_start < queue_end) {
                Point p = queue[queue_start++];
                int px = p.x;
                int py = p.y;
                int neighbors[4][2] = {{px-1, py}, {px+1, py}, {px, py-1}, {px, py+1}}; // get the pixel neighbors.
                for (int i = 0; i < 4; i++) {
                    int nx = neighbors[i][0];
                    int ny = neighbors[i][1];
                    if (nx >= 0 && nx < WIDTH && ny >= 0 && ny < HEIGHT) {// label all the neighbors.
                        if (mask[ny][nx] != 0 && label_map[ny][nx] == 0) {
                            label_map[ny][nx] = current_label; //set the label of the pixel in the map
                            queue[queue_end++] = (Point){nx, ny}; //add the indexesto the queue. and shift the end forward to repeat
                            size++; //increment the size of the blob.
                        }
                    }
                }
            }

            label_sizes[current_label] = size;// give the lable a size
            current_label++; // change the lable to the next blob.
            if (current_label >= MAX_LABELS) { //set a limit to the blob count to avoid forever looping.
                fprintf(stderr, "Warning: reached max blob count %d\n", MAX_LABELS);
                return;
            }
        }
    }
    // if the size of the blob in label map is smaller than min_size, then set all the values that map to the mask to 0
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
    float fb = b / 255.0f; // normalize all the rgb color channels to 0:1
    // The max of the channels is the Value or the V in HSV
    float max = fmaxf(fmaxf(fr, fg), fb); 
    // the minimum is used to compute the chroma or C.
    float min = fminf(fminf(fr, fg), fb);
    // The delta here refers to chroma, where C = V − min(R, G, B)
    float delta = max - min;
    // Assign v to the max value.
    *v = max;
    // saturation is 0 if the value is 0, otherwize its chroma/value
    *s = (max == 0) ? 0 : delta / max;
    //Hue is the tricky one.
    //if chroma is 0, then Hue is zero. (The image is a shade of gray, or R=G=B)
    if (delta == 0){
        *h = 0;
    } else if (max == fr){ //if max is red:
        float temp_h = ((fg - fb) / delta);
        if (temp_h < 0) temp_h += 6.0f;
        *h = 60.0f * temp_h;
        // h= 60*(((Green-blue)/chroma)%6)
        // i have had strange artifacts when using fmodf on pi, so We avoid it by using a very crude version.
        // temp_h will never be outside -1,1 when red is max. 
        // (r>g and r>b). delta = r - (g or b) => (g-b)/(r-b) => b<g => tp is +, bottom is +. 
        // top is smaller than bottom, result is always < 1.
        // for g<b => delta = r-g, (g-b)/(r-g) top is -, bottom is +,  also bottom is bigger than top, 
        // result is always > -1. so the range is -1 to 1. adding + 6 when the result is negative.
    } else if (max == fg){//if max is green
        *h = 60.0f * (((fb - fr) / delta) + 2.0f);
    } else{ // if max is blue
        *h = 60.0f * (((fr - fg) / delta) + 4.0f);
    }
    if (*h < 0) //wrap the hue arround if its negative 
    {
        *h += 360.0f;
    }
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
                    // just normal thresholding using HSV
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

        //Take the mutex
        pthread_mutex_lock(&green_mask_queue_mutex);
        // wait while the queue size is equal to the max of the queue
        while (green_mask_queue_count == GREEN_MASK_QUEUE_SIZE)
            pthread_cond_wait(&green_mask_queue_not_full, &green_mask_queue_mutex);// make sure to unlock the mutex while waiting.
        // copy the mask to the queue.
        memcpy(green_mask_queue[green_mask_queue_rear], green_mask, sizeof(green_mask));
        //Increment the queue tail
        green_mask_queue_rear = (green_mask_queue_rear + 1) % GREEN_MASK_QUEUE_SIZE;
        //Increment the current size of the queue
        green_mask_queue_count++;
        // Signal queue is not empty
        pthread_cond_signal(&green_mask_queue_not_empty);
        // Unlock the mutex
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

