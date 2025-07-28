#include <stdio.h>
#include <pigpio.h>
#include <unistd.h>

#define GPIO_PIN 24      
#define FREQ      40000  
#define STEP_US   50000  
#define STEP_SIZE 0.05  

int main() {
    if (gpioInitialise() < 0) {
        printf("pigpio init failed\n");
        return 1;
    }

    gpioSetMode(GPIO_PIN, PI_OUTPUT);

    int period_us = 1000000 / FREQ;

    double duty = 0.0;
    int direction = 1;

    while (1) {
        int high_us = (int)(period_us * duty);
        int low_us = period_us - high_us;

        gpioPulse_t pulses[2];
        pulses[0].gpioOn = (1 << GPIO_PIN);
        pulses[0].gpioOff = 0;
        pulses[0].usDelay = high_us;

        pulses[1].gpioOn = 0;
        pulses[1].gpioOff = (1 << GPIO_PIN);
        pulses[1].usDelay = low_us;

        gpioWaveClear();
        gpioWaveAddGeneric(2, pulses);
        int wave_id = gpioWaveCreate();

        if (wave_id >= 0) {
            gpioWaveTxSend(wave_id, PI_WAVE_MODE_REPEAT);
        } else {
            printf("Failed to create wave\n");
            break;
        }

        usleep(STEP_US); // Wait before changing duty

        gpioWaveTxStop();
        gpioWaveDelete(wave_id);

        // Update duty
        duty += STEP_SIZE * direction;
        if (duty >= 1.0) {
            duty = 1.0;
            direction = -1;
        } else if (duty <= 0.0) {
            duty = 0.0;
            direction = 1;
        }
    }

    gpioTerminate();
    return 0;
}
