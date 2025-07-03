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
#include "math.h"
#include "green_mask.h"
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


#ifdef PLATFORM_RPI
#define RESET_GPIO 23
char RXBuf[4];
char TXBuf[4];

int spiOpen(unsigned spiChan, unsigned spiBaud, unsigned spiFlags) {
  int fd;
  char spiMode;
  char spiBits = 8;
  char dev[32];
  spiMode = spiFlags & 3;
  spiBits = 8;
  sprintf(dev, "/dev/spidev0.%d", spiChan);
  if ((fd = open(dev, O_RDWR)) < 0) {
    return -1;
  }
  if (ioctl(fd, SPI_IOC_WR_MODE, &spiMode) < 0) {
    close(fd);
    return -2;
  }
  if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &spiBits) < 0) {
    close(fd);
    return -3;
  }
  if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &spiBaud) < 0) {
    close(fd);
    return -4;
  }
  return fd;
}

int spiXfer(int fd, char *txBuf, char *rxBuf, unsigned count) {
  int err;
  struct spi_ioc_transfer spi;
  memset(&spi, 0, sizeof(spi));
  spi.tx_buf = (unsigned long)txBuf;
  spi.rx_buf = (unsigned long)rxBuf;
  spi.len = count;
  spi.speed_hz = SPI_SPEED;
  spi.delay_usecs = 0;
  spi.bits_per_word = 8;
  spi.cs_change = 0;
  err = ioctl(fd, SPI_IOC_MESSAGE(1), &spi);
  return err;
}

void release_reset_gpio(){
    const char *chipname = "gpiochip0";
    struct gpiod_chip *chip = gpiod_chip_open_by_name(chipname);
    gpiod_chip_close(chip);

}
int reset_fpga(uint8_t *mem_interface_map) {
    (void)mem_interface_map;
    const char *chipname = "gpiochip0";
    struct gpiod_chip *chip;
    struct gpiod_line *line;
    chip = gpiod_chip_open_by_name(chipname);
    if (!chip) {
        perror("gpiod_chip_open_by_name");
        return 1;
    }
    line = gpiod_chip_get_line(chip, RESET_GPIO);
    if (!line) {
        perror("gpiod_chip_get_line");
        gpiod_chip_close(chip);
        return 1;
    }
    if (gpiod_line_request_output(line, "fpga-reset", 1) < 0) {
        perror("gpiod_line_request_output");
        gpiod_chip_close(chip);
        return 1;
    }
    gpiod_line_set_value(line, 0); usleep(2000);
    gpiod_line_set_value(line, 1); usleep(2000);
    gpiod_line_set_value(line, 0); usleep(1000);
    return 0;
}
#elif defined(PLATFORM_DE10)

void release_reset_gpio(){
    //reset GPIO is a physical button
}

int reset_fpga(uint8_t * mem_interface_map) {

    volatile uint32_t *base = (uint32_t *)mem_interface_map;
    uint16_t pitch_pwm  = PWM_MID_POINT;
    uint16_t yaw_pwm  = PWM_MID_POINT;
    base[0] = ((uint32_t)pitch_pwm << 16) | (uint32_t)yaw_pwm;
    return 0;
}
#endif

void read_write_encoder_pwm(int fd, uint8_t *mem_interface_map, uint16_t pitch_pwm, uint16_t yaw_pwm,
                            uint16_t *pitch_encoder, uint16_t *yaw_encoder)
{
#ifdef PLATFORM_RPI
    (void)mem_interface_map;
    TXBuf[0] = (pitch_pwm >> 8) & 0xFF;
    TXBuf[1] = pitch_pwm & 0xFF;
    TXBuf[2] = (yaw_pwm >> 8) & 0xFF;
    TXBuf[3] = yaw_pwm & 0xFF;
    spiXfer(fd, &TXBuf[0], RXBuf, 4);
    *pitch_encoder = ((uint16_t)RXBuf[0] << 8);
    *pitch_encoder |= RXBuf[1];
    *yaw_encoder = ((uint16_t)RXBuf[2] << 8);
    *yaw_encoder |= RXBuf[3];

#elif defined(PLATFORM_DE10)
    volatile uint32_t *base = (uint32_t *)mem_interface_map; // keep this volatile. especially if you compile -Ofast or -O3
    base[0] = ((uint32_t)pitch_pwm << 16) | (uint32_t)yaw_pwm;
    uint32_t encoder_vals = base[1];
    *pitch_encoder = (uint16_t)(encoder_vals >> 16);
    *yaw_encoder   = (uint16_t)(encoder_vals & 0xFFFF);
#endif
}

void init_fpga(int fd, uint16_t * pitch_encoder, uint16_t * yaw_encoder, uint8_t *mem_interface_map){
    reset_fpga(mem_interface_map);
    read_write_encoder_pwm(fd, mem_interface_map, PWM_MID_POINT, PWM_MID_POINT, pitch_encoder, yaw_encoder);
}

void homing_sequence(int fd, uint8_t *mem_interface_map, uint16_t * p_encoder_settings, uint16_t *y_encoder_settings){
    uint16_t pitch_pwm = get_pwm_value(-1, 0); 
    uint16_t yaw_pwm = get_pwm_value(-1, 0);
    read_write_encoder_pwm(fd, mem_interface_map, pitch_pwm, yaw_pwm, &p_encoder_settings[0],&y_encoder_settings[0]);
    printf("[HOME] Starting values for encoders %d, %d\n",p_encoder_settings[0], y_encoder_settings[0]);
    
    pitch_pwm = get_pwm_value(-1, 128); 
    yaw_pwm = get_pwm_value(-1, 512);
    for (int i = 0; i < 1000; i++)
    {
        read_write_encoder_pwm(fd, mem_interface_map, pitch_pwm, yaw_pwm, &p_encoder_settings[1],&y_encoder_settings[1]);
        usleep(100);
    }
    pitch_pwm = get_pwm_value(-1, 0); 
    yaw_pwm = get_pwm_value(-1, 0);
    read_write_encoder_pwm(fd, mem_interface_map, pitch_pwm, yaw_pwm, &p_encoder_settings[1],&y_encoder_settings[1]);
    usleep(1000);
    read_write_encoder_pwm(fd, mem_interface_map, pitch_pwm, yaw_pwm,  &p_encoder_settings[1], &y_encoder_settings[1]);
    printf("[HOME] Begin values for encoders %d, %d\n",p_encoder_settings[1], y_encoder_settings[1]);
    

    pitch_pwm = get_pwm_value(1, 128); 
    yaw_pwm = get_pwm_value(1, 512);
    for (int i = 0; i < 1000; i++)
    {
        read_write_encoder_pwm(fd, mem_interface_map, pitch_pwm, yaw_pwm,  &p_encoder_settings[2], &y_encoder_settings[2]);
        usleep(100);
    }
    pitch_pwm = get_pwm_value(1, 0); 
    yaw_pwm = get_pwm_value(1, 0);
    read_write_encoder_pwm(fd, mem_interface_map, pitch_pwm, yaw_pwm, &p_encoder_settings[2], &y_encoder_settings[2]);
    usleep(1000);
    read_write_encoder_pwm(fd, mem_interface_map, pitch_pwm, yaw_pwm, &p_encoder_settings[2], &y_encoder_settings[2]);
    printf("[HOME] End values for encoders %d, %d\n",p_encoder_settings[2], y_encoder_settings[2]);

    printf("[HOME] Encoder range %d, %d\n",p_encoder_settings[2] - p_encoder_settings[1], y_encoder_settings[2] - y_encoder_settings[1]);

}

void compute_hv_fov(double dfov_deg, double *hfov, double *vfov)
{
    double dfov = dfov_deg * M_PI / 180.0;  // e.g. 55° → 0.95993 rad
    double D = sqrt(WIDTH*WIDTH + HEIGHT*HEIGHT);  // e.g. ≈734.37
    double f = (D / 2.0) / tan(dfov / 2.0);         // e.g. ≈703.95
    *hfov = 2.0 * atan((WIDTH  / 2.0) / f);   // ≈ 0.8520 rad
    *vfov = 2.0 * atan((HEIGHT / 2.0) / f);   // ≈ 0.4996 rad
}


void get_rad_from_encoder(
    double  *pitch_angle,
    double  *yaw_angle,
    uint16_t *pitch_encoder_settings,
    uint16_t *yaw_encoder_settings
)
{
    // encoder_settings: start, begin, end, current
    uint16_t pitch_begin   = pitch_encoder_settings[1];
    uint16_t pitch_end     = pitch_encoder_settings[2];
    uint16_t pitch_current = pitch_encoder_settings[3];

    uint16_t yaw_begin     = yaw_encoder_settings[1];
    uint16_t yaw_end       = yaw_encoder_settings[2];
    uint16_t yaw_current   = yaw_encoder_settings[3];

    if (pitch_begin == pitch_end) {
        *pitch_angle = PITCH_MIN_RAD;
    } else {
        double ratio = (double)(pitch_current - pitch_begin) / (double)(pitch_end - pitch_begin);
        double pitch_rad = PITCH_MIN_RAD + ratio * (PITCH_MAX_RAD - PITCH_MIN_RAD);
        *pitch_angle = pitch_rad;
    }

    if (yaw_begin == yaw_end) {
        *yaw_angle = YAW_MIN_RAD;
    } else {
        double ratio = (double)(yaw_current - yaw_begin) / (double)(yaw_end - yaw_begin);
        double yaw_rad = YAW_MIN_RAD + ratio * (YAW_MAX_RAD - YAW_MIN_RAD);
        *yaw_angle = yaw_rad;
    }
}
