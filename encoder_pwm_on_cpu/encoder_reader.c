#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <gpiod.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <stdatomic.h>

#define GPIO_CHIP "/dev/gpiochip0"
#define PIN_A 20 // Encoder A channel on GPIO20
#define PIN_B 21 // Encoder B channel on GPIO21

atomic_int position = 0;
struct gpiod_line *line_a, *line_b;

void *interrupt_handler(void *arg)
{
    struct gpiod_line_event event;

    while (1)
    {
        if (gpiod_line_event_wait(line_a, NULL) == 1)
        {
            gpiod_line_event_read(line_a, &event);

            int a = gpiod_line_get_value(line_a);
            int b = gpiod_line_get_value(line_b);

            if (a == b)
            {
                atomic_fetch_add(&position, 1);
            }
            else
            {
                atomic_fetch_sub(&position, 1);
            }
        }
    }

    return NULL;
}

int main()
{
    struct gpiod_chip *chip;
    pthread_t thread;

    chip = gpiod_chip_open(GPIO_CHIP);
    if (!chip)
    {
        perror("Failed to open GPIO chip, check if claimed by other processes");
        return 1;
    }

    line_a = gpiod_chip_get_line(chip, PIN_A);
    line_b = gpiod_chip_get_line(chip, PIN_B);

    gpiod_line_request_both_edges_events(line_a, "encoder");
    gpiod_line_request_input(line_b, "encoder");

    struct sched_param sched;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    sched.sched_priority = 80; // We set the priority higher than all system priority tasks (20)
    pthread_attr_setschedparam(&attr, &sched);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);

    pthread_create(&thread, &attr, interrupt_handler, NULL);
    struct timespec ts;
    while (1)
    {
        clock_gettime(CLOCK_MONOTONIC, &ts);
        //Div by 2 beacuse we request both edges, Rising and falling
        // So the pulse count is actually double the correct one.
        printf("Time: %ld.%06ld s | Position: %d\n",
           ts.tv_sec, ts.tv_nsec / 1000, (atomic_load(&position) / 2));
        usleep(1000*100);
    }

    gpiod_chip_close(chip);
    return 0;
}
