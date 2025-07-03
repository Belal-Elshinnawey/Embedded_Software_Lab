#ifndef HW_INTERFACE_H
#define HW_INTERFACE_H

#include <stdint.h>
#include <stdbool.h>
#ifdef PLATFORM_RPI
#include <getopt.h>
#include <gpiod.h>
#include <linux/spi/spidev.h>
#include <linux/types.h>
#include <sys/ioctl.h>
#elif defined(PLATFORM_DE10)
#include <sys/mman.h>
#include "soc_system.h"
#endif


#define MAP_SIZE HPS_0_ARM_A9_0_MEM_INTERFACE_0_SPAN
#define MAP_BASE HPS_0_ARM_A9_0_MEM_INTERFACE_0_BASE



#ifdef PLATFORM_RPI
int spiXfer(int fd, char *txBuf, char *rxBuf, unsigned count);
int spiOpen(unsigned spiChan, unsigned spiBaud, unsigned spiFlags);
#elif defined(PLATFORM_DE10)
#endif

void read_write_encoder_pwm(int fd, uint8_t *mem_interface_map, uint16_t pitch_pwm, uint16_t yaw_pwm,
                            uint16_t *pitch_encoder, uint16_t *yaw_encoder);
void init_fpga(int fd, uint16_t * pitch_encoder, uint16_t * yaw_encoder, uint8_t *mem_interface_map);

static inline uint16_t get_pwm_value(int dir, uint16_t  requested_pwm1){
    return (uint16_t)(((uint16_t)(PWM_MID_POINT)) +( ((int)requested_pwm1) * dir));
}
void release_reset_gpio();

void homing_sequence(int fd, uint8_t *mem_interface_map, uint16_t *p_encoder_settings, uint16_t *y_encoder_settings);

int reset_fpga(uint8_t * mem_interface_map);

void get_rad_from_encoder(
    double  *pitch_angle,
    double  *yaw_angle,
    uint16_t *pitch_encoder_settings,
    uint16_t *yaw_encoder_settings
);
void compute_hv_fov(double dfov_deg, double *hfov_deg, double *vfov_deg);

#endif
