#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include "image_processor.h"
#include <pthread.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "rt_utils.h"
#include "hw_interface.h"
#include "pitch_xxsubmod.h"
#include "yaw_xxsubmod.h"
#include "green_mask.h"
#include "math.h"
#include "start_com_queue.h"
#include "profiling.h"


typedef enum
{
    STATE_READY,
    STATE_INIT,
    STATE_HOME,
    STATE_START,
    STATE_END
} State;

volatile State current_state = STATE_READY;
volatile bool start_thread_active = false;
pthread_t start_thread;
pthread_mutex_t start_thread_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t start_cond = PTHREAD_COND_INITIALIZER;

static uint16_t yaw_encoder_settings[4];
static uint16_t pitch_encoder_settings[4];

static int fd;
static uint8_t *mem_interface_map;

void state_ready()
{
    printf("[READY] Awaiting 'init'\n");
}

void state_init(int fd, uint8_t *mem_interface_map, uint16_t *pitch_encoder, uint16_t *yaw_encoder)
{
    printf("[INIT] Starting init process\n");
    init_fpga(fd, pitch_encoder, yaw_encoder, mem_interface_map);
    printf("[INIT] init Finished Waiting for homing sequence\n");
}

void state_home(int fd, uint8_t *mem_interface_map, uint16_t *pitch_encoder, uint16_t *yaw_encoder)
{
    printf("[HOME] Starting Homing sequence\n");
    homing_sequence(fd, mem_interface_map, pitch_encoder, yaw_encoder);
    printf("[HOME] Finished Homing sequence, Max yaw: %d, Max pitch: %d\n", *pitch_encoder, *yaw_encoder);
}

void state_end(int fd, uint8_t *mem_interface_map, uint16_t *pitch_encoder, uint16_t *yaw_encoder)
{
    printf("[END] Stopping system and exiting\n");
    read_write_encoder_pwm(fd, mem_interface_map, PWM_MID_POINT, PWM_MID_POINT, pitch_encoder, yaw_encoder);
    release_reset_gpio();
}

void *start_thread_func(void *arg)
{
    set_realtime_priority(pthread_self(), 85);
    const double f     = 4.0;      // focal length in mm
    const double sw    = 3.58;     // sensor width in mm
    const double sh    = 2.02;     // sensor height in mm
    const double hfov_rad = 2.0 * atan((sw / 2.0) / f);
    const double vfov_rad = 2.0 * atan((sh / 2.0) / f);
    const double hfov_deg = hfov_rad * (180.0 / M_PI);
    const double vfov_deg = vfov_rad * (180.0 / M_PI);
    const double half_width = (WIDTH / 2.0);
    const double half_height = (HEIGHT / 2.0);
    const double D = sqrt(sw * sw + sh * sh);
    const double dfov_rad = 2.0 * atan((D / 2.0) / f);
    const double dfov_deg = dfov_rad * (180.0 / M_PI);
    const double hscale = (hfov_rad / WIDTH);
    const double vscale = (vfov_rad / HEIGHT);
    printf("dfov %f %f\n", dfov_rad, dfov_deg);
    printf("f %f \n", f);
    printf("hfov rad %f \n", hfov_rad);
    printf("vfov rad %f \n", vfov_rad);
    printf("hfov deg %f \n", hfov_deg);
    printf("vfov deg %f \n", vfov_deg);
    printf("half_width %f \n", half_width);
    printf("half_height %f \n", half_height);
    printf("hscale %f \n", hscale);
    printf("vscale %f \n", vscale);
    double yaw_set_point = 0;
    double pitch_set_point = 0;
    uint16_t pitch_pwm = PWM_MID_POINT;
    uint16_t yaw_pwm = PWM_MID_POINT;
    uint16_t requested_pitch_pwm =0;
    uint16_t requested_yaw_pwm =0;
    int yaw_dir = 0;
    int pitch_dir = 0;
    double yaw_position_current = 0;
    double pitch_position_current = 0;
    YAW_XXDouble yaw_inputs[3];
    YAW_XXDouble yaw_outputs[3];
    PITCH_XXDouble pitch_inputs[3];
    PITCH_XXDouble pitch_outputs[3];
    COM com_val = {0};
    float pixel_err_x =  0.0;
    float pixel_err_y =  0.0;
    TimeRecord thread_timing;
#if defined(SINGLE_SHOT_TEST) && SINGLE_SHOT_TEST == 1
    struct timespec single_shot_start_time;
    struct timespec single_shot_end_time;
#endif    

    while (1)
    {
        clock_gettime(CLOCK_MONOTONIC, &(thread_timing.start_time));
        pthread_mutex_lock(&start_thread_lock);
        while (!start_thread_active)
        {
            pthread_cond_wait(&start_cond, &start_thread_lock);
        }

        pthread_mutex_unlock(&start_thread_lock);
        read_write_encoder_pwm(fd, mem_interface_map, pitch_pwm, yaw_pwm, &pitch_encoder_settings[3], &yaw_encoder_settings[3]);
        get_rad_from_encoder(
            &pitch_position_current,
            &yaw_position_current,
            pitch_encoder_settings,
            yaw_encoder_settings
        );
        printf("Starting Control loop, Current Pos: yaw:%f pitch:%f", yaw_position_current, pitch_position_current);
        yaw_inputs[0] = yaw_position_current;     /* in */
        yaw_inputs[1] = yaw_position_current;     /* position */
        yaw_outputs[0] = 0.0;                     /* corr */
        yaw_outputs[1] = 0.0;                     /* out */
        pitch_inputs[1] = pitch_position_current; /* in */
        pitch_inputs[2] = pitch_position_current; /* position */
        pitch_outputs[0] = 0.0;                   /* corr */
        pitch_outputs[0] = 0.0;                   /* out */
        YAW_XXInitializeSubmodel(yaw_inputs, yaw_outputs, yaw_xx_time);
        PITCH_XXInitializeSubmodel(pitch_inputs, pitch_outputs, pitch_xx_time);
#if defined(SINGLE_SHOT_TEST) && SINGLE_SHOT_TEST == 1
        clock_gettime(CLOCK_MONOTONIC, &single_shot_start_time);
#endif
        while (start_thread_active)
        {
            clock_gettime(CLOCK_MONOTONIC, &(thread_timing.start_time));
            if (spsc_dequeue(&com_queue, &com_val) && com_val.com_x > 0.0f && com_val.com_y > 0.0f)
            {
                pixel_err_x = half_width - com_val.com_x;
                pixel_err_y = half_height - com_val.com_y;

                // printf("[START] perr %.2f\n", pixel_err_y);

                // no fancy fabs that works sometimes but if you use -Ofast it outputs garbage.
                if (pixel_err_x <= -50.0 || pixel_err_x >= 50.0){
                    yaw_set_point = yaw_position_current +  (pixel_err_x * YAW_RAD_PER_PIXEL);
                }

                if (pixel_err_y <= -50.0 || pixel_err_y >= 50.0){
                    pitch_set_point = pitch_position_current +  (pixel_err_y * PITCH_RAD_PER_PIXEL);
                }
                //clip for safty. causes jitter but the result is way worse if not used.
                yaw_set_point = yaw_set_point > YAW_MAX_RAD ? yaw_position_current : yaw_set_point < YAW_MIN_RAD ? yaw_position_current : yaw_set_point;
                pitch_set_point = pitch_set_point > PITCH_MAX_RAD ? pitch_position_current : pitch_set_point < PITCH_MIN_RAD ? pitch_position_current : pitch_set_point;
                

            }

#if defined(SINGLE_SHOT_TEST) && SINGLE_SHOT_TEST == 1
            yaw_set_point = YAW_MAX_RAD/2;
            pitch_set_point = PITCH_MAX_RAD/2;
            if(fabs(yaw_set_point - yaw_position_current) < 0.1 && fabs(pitch_set_point - pitch_position_current) < 0.1){
                clock_gettime(CLOCK_MONOTONIC, &single_shot_end_time);
                 long diff_ms = (single_shot_end_time.tv_sec - single_shot_start_time.tv_sec) * 1000 +
                   (single_shot_end_time.tv_nsec - single_shot_start_time.tv_nsec) / 1000000;
                printf("Time difference: %ld ms\n", diff_ms);
                while (true)
                {
                    usleep(1000);
                }
            }
#endif
            // printf("[START] Set Point (%.2f, %.2f) Cur Pos (%.2f, %.2f)  COM (%.2f, %.2f)\n", 
            // yaw_set_point, pitch_set_point, yaw_position_current, pitch_position_current, com_val.com_x, com_val.com_y);

            yaw_inputs[0] = yaw_set_point;
            yaw_inputs[1] = yaw_position_current;
            pitch_inputs[1] = pitch_set_point;
            pitch_inputs[2] = pitch_position_current;

            // printf("[START] set point (%.2f, %.2f)\n", yaw_set_point, pitch_set_point);
            // printf("Current Pos: yaw:%f pitch:%f\n", yaw_position_current, pitch_position_current);

            YAW_XXCalculateSubmodel(yaw_inputs, yaw_outputs, yaw_xx_time);
            PITCH_XXCalculateSubmodel(pitch_inputs, pitch_outputs, pitch_xx_time);
            // printf("[START] 20SIM Model (%.2f, %.2f)\n", yaw_outputs[1], pitch_outputs[0]);


            // printf("[START] 20SIM Model (%.2f, %.2f)\n", yaw_outputs[1], pitch_outputs[0]);
            requested_yaw_pwm = (uint16_t)(yaw_outputs[1] * (yaw_outputs[1] <= 0.0f ? YAW_PWM_MIN : YAW_PWM_MAX));
            requested_pitch_pwm = (uint16_t)((pitch_outputs[0]) * (pitch_outputs[0] <= 0.0f ? PITCH_PWM_MIN : PITCH_PWM_MAX));
            // printf("[START] 20SIM Model Requested PWM(%d, %d)\n", requested_yaw_pwm, requested_pitch_pwm);

            yaw_dir = (yaw_outputs[1]) <= 0.0f ? -1 : 1;
            pitch_dir = (pitch_outputs[0]) <= 0.0f ? -1 : 1;
            pitch_pwm = get_pwm_value(pitch_dir,requested_pitch_pwm);
            yaw_pwm = get_pwm_value(yaw_dir,requested_yaw_pwm);
            read_write_encoder_pwm(fd, mem_interface_map, pitch_pwm, yaw_pwm, &pitch_encoder_settings[3], &yaw_encoder_settings[3]);
            get_rad_from_encoder(
                &pitch_position_current,
                &yaw_position_current,
                pitch_encoder_settings,
                yaw_encoder_settings
            );

            if (!start_thread_active)
            {
                YAW_XXTerminateSubmodel(yaw_inputs, yaw_outputs, yaw_xx_time);
                PITCH_XXTerminateSubmodel(pitch_inputs, pitch_outputs, pitch_xx_time);
                return NULL;
            }
            usleep(5000);
            clock_gettime(CLOCK_MONOTONIC, &(thread_timing.end_time));
            profile_thread(3, thread_timing);
        }
    }
    return NULL;
}

void state_start()
{
    pthread_mutex_lock(&start_thread_lock);
    start_thread_active = true;
    pthread_cond_signal(&start_cond);
    pthread_mutex_unlock(&start_thread_lock);
    printf("[START] Started control thread\n");
}

void stop_start_thread()
{
    pthread_mutex_lock(&start_thread_lock);
    start_thread_active = false;
    pthread_mutex_unlock(&start_thread_lock);
    printf("[START] Control thread suspended\n");
}

int main()
{
    fd = -1;
    mem_interface_map = NULL;
    spsc_init(&com_queue);
#ifdef PLATFORM_RPI
    fd = spiOpen(1, SPI_SPEED, 0);
    if (fd < 0)
    {
        perror("Couldn't SPI device");
        return -1;
    }
#elif defined(PLATFORM_DE10)
    fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0)
    {
        perror("Couldn't Mem device");
        return -1;
    }
    mem_interface_map = (uint8_t *)mmap(
        NULL,
        MAP_SIZE,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        fd,
        MAP_BASE);
    if (mem_interface_map == MAP_FAILED)
    {
        perror("Couldn't mmap device");
        close(fd);
        return -1;
    }
#endif

    start_image_processor();
    char *input;
    pthread_create(&start_thread, NULL, start_thread_func, NULL);
#ifdef PLATFORM_RPI
    pin_thread_to_core(start_thread, 3);
#elif defined(PLATFORM_DE10)
    pin_thread_to_core(start_thread, 1);
#endif

    while ((input = readline("> ")) != NULL)
    {
        if (strlen(input) > 0)
            add_history(input);

        if (strcmp(input, "init") == 0 && current_state == STATE_READY)
        {
            current_state = STATE_INIT;
        }
        else if (strcmp(input, "home") == 0 && current_state == STATE_INIT)
        {
            current_state = STATE_HOME;
        }
        else if (strcmp(input, "start") == 0 && current_state == STATE_HOME)
        {
            current_state = STATE_START;
        }
        else if (strcmp(input, "end") == 0 && (current_state == STATE_HOME || current_state == STATE_START))
        {
            stop_start_thread();
            current_state = STATE_END;
        }
        else
        {
            printf("Invalid transition or command.\n");
        }

        switch (current_state)
        {
        case STATE_READY:
            state_ready();
            break;
        case STATE_INIT:
            state_init(fd, mem_interface_map, &pitch_encoder_settings[3], &yaw_encoder_settings[3]);
            break;
        case STATE_HOME:
            state_home(fd, mem_interface_map, pitch_encoder_settings, yaw_encoder_settings);
            break;
        case STATE_START:
            state_start();
            break;
        case STATE_END:
            state_end(fd, mem_interface_map, &pitch_encoder_settings[3], &yaw_encoder_settings[3]);
            break;
        }
        if (current_state == STATE_END)
        {
            free(input);
            break;
        }
        free(input);
        usleep(100000);
    }
    pthread_cancel(start_thread);
    pthread_join(start_thread, NULL);

    return 0;
}

// make PROFILE=0 PLATFORM=RPI
// make PROFILE=1 PLATFORM=DE10
// make clean 