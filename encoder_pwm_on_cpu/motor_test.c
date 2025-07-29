#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <gpiod.h>
#include <pthread.h>
#include <pigpio.h>
#include <stdatomic.h>
#include <math.h>
#include <stdarg.h>
#include <signal.h>

#define GPIO_CHIP "/dev/gpiochip0"
#define PIN_A 20
#define PIN_B 21

#define PWM_PIN     16
#define DIR_PIN_1   23
#define DIR_PIN_2   24
#define FREQ        20000
#define STEP_US     10000

atomic_int position = 0;
struct gpiod_line *line_a, *line_b;
struct gpiod_chip *chip = NULL;
int last_a = 0, last_b = 0;
pthread_mutex_t state_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t pid_lock = PTHREAD_MUTEX_INITIALIZER;

double kp = 0.00, ki = 0.000, kd = 0.00;
int setpoint = 200;

volatile sig_atomic_t stop_requested = 0;

void handle_signal(int sig) {
    stop_requested = 1;
    printf("\n[INFO] Caught signal %d, shutting down...\n", sig);
}

void process_transition(int new_a, int new_b) {
    int prev = (last_a << 1) | last_b;
    int curr = (new_a << 1) | new_b;
    int delta = 0;

    switch ((prev << 2) | curr) {
        case 0b0001: case 0b0111: case 0b1110: case 0b1000: delta = 1; break;
        case 0b0010: case 0b0100: case 0b1101: case 0b1011: delta = -1; break;
        default: break;
    }
    if (delta) atomic_fetch_add(&position, delta);
    last_a = new_a;
    last_b = new_b;
}

void* watch_line(void* arg) {
    struct gpiod_line *line = (struct gpiod_line *)arg;
    struct gpiod_line_event event;

    while (!stop_requested) {
        if (gpiod_line_event_wait(line, NULL) == 1 &&
            gpiod_line_event_read(line, &event) == 0) {
            pthread_mutex_lock(&state_lock);
            int a = gpiod_line_get_value(line_a);
            int b = gpiod_line_get_value(line_b);
            process_transition(a, b);
            pthread_mutex_unlock(&state_lock);
        }
    }
    return NULL;
}

void pwm_write(double duty, int direction) {
    int period_us = 1000000 / FREQ;
    int high_us = (int)(period_us * duty);
    int low_us = period_us - high_us;
    if (direction == 0){
        gpioWrite(DIR_PIN_1, 0);
        gpioWrite(DIR_PIN_2, 0);
    }else{
        gpioWrite(DIR_PIN_1, direction == 1 ? 0 : 1);
        gpioWrite(DIR_PIN_2, direction == 1 ? 1 : 0);
    }
    gpioPulse_t pulses[2];
    pulses[0].gpioOn = (1 << PWM_PIN);
    pulses[0].gpioOff = 0;
    pulses[0].usDelay = high_us;

    pulses[1].gpioOn = 0;
    pulses[1].gpioOff = (1 << PWM_PIN);
    pulses[1].usDelay = low_us;
    gpioWaveClear();
    gpioWaveAddGeneric(2, pulses);
    int wave_id = gpioWaveCreate();
    if (wave_id >= 0) gpioWaveTxSend(wave_id, PI_WAVE_MODE_REPEAT);
}

void* controller_thread(void* arg) {
    double integral = 0.0, last_error = 0.0;
    double max_duty = 0.65, min_duty = 0.0;

    const double dt = STEP_US / 1e6;
    while (!stop_requested) {

        int pos = atomic_load(&position);
        double error = (double)(setpoint - pos);
        if(error <= 15.0 && error >=-15){
            error = 0;
        }
        integral += error * dt;
        double derivative = (error - last_error) / dt;
        last_error = error;
        double output = kp * error + ki * integral + kd * derivative;
        int dir = (output >= 0) ? 1 : -1;
        if (error == 0){
            dir =0;
        }
        double duty = (output >= 0) ? output: -output;

        if (duty > max_duty) duty = max_duty;

        pwm_write(duty, dir);

        printf("Pos: %d | Err: %.2f | Duty: %.2f | Dir: %d | P: %.2f | I: %.2f | D: %.2f\n",
                    pos, error, duty, dir, kp * error, ki * integral, kd * derivative);

        usleep(STEP_US);
    }
    return NULL;
}

// -----------------------------
// Cleanup
// -----------------------------
void cleanup() {
    printf("[INFO] Cleaning up...\n");

    gpioWaveTxStop();
    gpioTerminate();

    if (line_a) gpiod_line_release(line_a);
    if (line_b) gpiod_line_release(line_b);
    if (chip) gpiod_chip_close(chip);

    printf("[INFO] Done.\n");
}

int main(int argc, char *argv[]) {
    pthread_t thread_a, thread_b, control_thread;

    if (argc != 5) {
        fprintf(stderr, "Usage: %s <kp> <ki> <kd> <setpoint>\n", argv[0]);
        return 1;
    }
    kp = atof(argv[1]);
    ki = atof(argv[2]);
    kd = atof(argv[3]);
    setpoint = atoi(argv[4]);
    printf("Starting with: kp = %.6f, ki = %.6f, kd = %.6f, setpoint = %d\n",
           kp, ki, kd, setpoint);
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    chip = gpiod_chip_open(GPIO_CHIP);
    if (!chip) {
        perror("Failed to open GPIO chip");
        return 1;
    }
    line_a = gpiod_chip_get_line(chip, PIN_A);
    line_b = gpiod_chip_get_line(chip, PIN_B);
    gpiod_line_request_both_edges_events(line_a, "encoder");
    gpiod_line_request_both_edges_events(line_b, "encoder");
    last_a = gpiod_line_get_value(line_a);
    last_b = gpiod_line_get_value(line_b);
    if (gpioInitialise() < 0) {
        printf("pigpio init failed\n");
        return 1;
    }
    gpioSetMode(PWM_PIN, PI_OUTPUT);
    gpioSetMode(DIR_PIN_1, PI_OUTPUT);
    gpioSetMode(DIR_PIN_2, PI_OUTPUT);
    pthread_create(&thread_a, NULL, watch_line, (void *)line_a);
    pthread_create(&thread_b, NULL, watch_line, (void *)line_b);
    pthread_create(&control_thread, NULL, controller_thread, NULL);
    pthread_join(control_thread, NULL);
    pthread_join(thread_a, NULL);
    pthread_join(thread_b, NULL);
    cleanup();
    return 0;
}
