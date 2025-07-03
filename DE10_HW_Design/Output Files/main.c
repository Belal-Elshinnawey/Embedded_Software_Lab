#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdbool.h>
#include "soc_system.h"

#define MAP_SIZE HPS_0_ARM_A9_0_MEM_INTERFACE_0_SPAN
#define MAP_BASE HPS_0_ARM_A9_0_MEM_INTERFACE_0_BASE

#define HPS_0_ARM_A9_0_MEM_INTERFACE_0_COMPONENT_TYPE mem_interface
#define HPS_0_ARM_A9_0_MEM_INTERFACE_0_COMPONENT_NAME mem_interface_0

void reset_fpga_state(uint8_t *mem_interface_map){
    volatile uint32_t *base = (uint32_t *)mem_interface_map;
    base[2] = 1;
    usleep(10000);
    base[2] = 0;
    usleep(10000);
}



int main(int argc, char **argv)
{
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0)
    {
        perror("Couldn't open /dev/mem");
        return -1;
    }



    uint8_t *mem_interface_map = (uint8_t *)mmap(
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

    // reset_fpga_state(mem_interface_map);

    volatile uint32_t *base = (uint32_t *)mem_interface_map;

    uint16_t pitch_pwm = 2048 + 1024;
    uint16_t yaw_pwm = 2048 - 1024;
    uint32_t combined_pwm = ((uint32_t)pitch_pwm << 16) | yaw_pwm;

    base[0] = combined_pwm;         // Write to PWM register (offset 0x00)
    uint32_t encoder_vals = base[1]; // Read from Encoder register (offset 0x04)
    
    printf("Wrote PWM: pitch=%u, yaw=%u (combined=0x%08X)\n", pitch_pwm, yaw_pwm, combined_pwm);
    for (size_t i = 0; i < MAP_SIZE/4; i++)
    {
        base[i] = combined_pwm;         // Write to PWM register (offset 0x00)
    }
    while(true){
        encoder_vals = base[1];
        base[1] = combined_pwm;
        uint16_t encoder_low  = (uint16_t)(encoder_vals & 0xFFFF);         // lower 16 bits
        uint16_t encoder_high = (uint16_t)((encoder_vals >> 16) & 0xFFFF); 
        printf("Read Encoder Value: E1: %d %d\n", encoder_low, encoder_high);
        usleep(100 * 1000);
    }

    munmap((void *)mem_interface_map, MAP_SIZE);
    
    close(fd);
    return 0;
}
