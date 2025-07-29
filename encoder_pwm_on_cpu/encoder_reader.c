#include <gpiod.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>

#define GPIO_CHIP "/dev/gpiochip0"
#define PIN_A 20
#define PIN_B 21

atomic_int position = 0;
struct gpiod_line *line_a, *line_b;

int last_a = 0;
int last_b = 0;
pthread_mutex_t state_lock = PTHREAD_MUTEX_INITIALIZER;

void process_transition(int new_a, int new_b)
{
    int prev = (last_a << 1) | last_b;
    int curr = (new_a << 1) | new_b;

    int delta = 0;
    switch ((prev << 2) | curr) {
        case 0b0001:
        case 0b0111:
        case 0b1110:
        case 0b1000: delta = 1; break;
        case 0b0010:
        case 0b0100:
        case 0b1101:
        case 0b1011: delta = -1; break;
        default: break;
    }

    if (delta != 0){
        atomic_fetch_add(&position, delta);
    }

    last_a = new_a;
    last_b = new_b;
}

void *watch_line(void *arg)
{
    struct gpiod_line *line = (struct gpiod_line *)arg;
    struct gpiod_line_event event;

    while (1)
    {
        if (gpiod_line_event_wait(line, NULL) == 1 &&
            gpiod_line_event_read(line, &event) == 0)
        {
            pthread_mutex_lock(&state_lock);
            int a = gpiod_line_get_value(line_a);
            int b = gpiod_line_get_value(line_b);
            process_transition(a, b);
            pthread_mutex_unlock(&state_lock);
        }
    }

    return NULL;
}

int main()
{
    struct gpiod_chip *chip;
    pthread_t thread_a, thread_b;

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

    pthread_create(&thread_a, NULL, watch_line, (void *)line_a);
    pthread_create(&thread_b, NULL, watch_line, (void *)line_b);

    struct timespec ts;
    while (1) {
        clock_gettime(CLOCK_MONOTONIC, &ts);
        printf("Time: %ld.%06ld s | Position: %d\n",
               ts.tv_sec, ts.tv_nsec / 1000, atomic_load(&position));
        usleep(100 * 1000);
    }
    gpiod_chip_close(chip);
    return 0;
}
