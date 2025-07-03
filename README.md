![Ball tracking](https://github.com/Belal-Elshinnawey/Embedded_Software_Lab/blob/main/ball_tracking.gif)

Overview

This project contains the code for the Embedded Software Lab. The repository is organized into three main directories:

Directory Structure

- DE10_HW_Design
  Contains the hardware design files for the DE10 Nano board.

- esl_lab_full_app
  Contains the C application source code to run on both Raspberry Pi and DE10 Nano platforms.
  To build, go into the directory and run `make`.
  The directory contains a `config.mk` file with the following configurable options:

  ## Enable thread profiling
  PROFILE= 0
  Enables or disables profiling of thread execution times for performance analysis.

  ## Enable UDP video server
  DEBUG_VIDEO= 1
  Enables a UDP server to stream video frames for debugging purposes.

  ## Target platform: RPI or DE10
  PLATFORM= DE10
  Specifies the target hardware platform to build for, either Raspberry Pi (RPI) or DE10 Nano (DE10).

  ## Video and detection configuration
  CFLAGS += -DWIDTH=320
  CFLAGS += -DHEIGHT=240
  Set the resolution width and height of the video frames processed.

  CFLAGS += -DGREEN_MASK_QUEUE_SIZE=5
  Sets the size of the queue used for green mask image buffering.

  CFLAGS += -DGREEN_H_MIN=90.0
  CFLAGS += -DGREEN_H_MAX=180.0
  CFLAGS += -DGREEN_S_MIN=0.25
  CFLAGS += -DGREEN_V_MIN=0.1
  Configure HSV color space thresholds to filter green pixels in the image.

  CFLAGS += -DMIN_GREEN_PIXELS=1000
  Minimum number of green pixels required to consider an object detected.

  CFLAGS += -DFRAME_RATE=30
  Target frame rate of the video processing pipeline.

  CFLAGS += -DVIDEO_DEVICE="/dev/video7"
  The device file path for the video input source.

  CFLAGS += -DGREEN_MASK_DEBUG_PORT=5000
  CFLAGS += -DCOM_THREAD_DEBUG_PORT=5001
  UDP ports used for debugging the green mask and center of mass threads.

  ## Hardware configuration
  CFLAGS += -DSPI_SPEED=1000
  SPI communication speed setting.

  CFLAGS += -DPWM_MID_POINT=2048
  Midpoint value for PWM signal generation.

  CFLAGS += -DPITCH_MIN_RAD=0.2617993878
  CFLAGS += -DPITCH_MAX_RAD=2.3561944902
  Minimum and maximum pitch angles in radians.

  CFLAGS += -DYAW_MIN_RAD=0.0
  CFLAGS += -DYAW_MAX_RAD=3.1415926536
  Minimum and maximum yaw angles in radians.

  CFLAGS += -DYAW_PWM_MIN=-2048.0f
  CFLAGS += -DYAW_PWM_MAX=2048.0f
  CFLAGS += -DPITCH_PWM_MIN=-512.0f
  CFLAGS += -DPITCH_PWM_MAX=512.0f
  PWM signal ranges for yaw and pitch.

  CFLAGS += -DPITCH_RAD_PER_PIXEL=0.001f
  CFLAGS += -DYAW_RAD_PER_PIXEL=0.001f
  Conversion factors from pixels to radians for pitch and yaw.

  CFLAGS += -DSINGLE_SHOT_TEST=0
  Enables or disables single-shot test mode.

- ICO_HW_Design
  Contains the hardware design files for the ICO board.

- pi_profile
  Contains performance and testing data collected from experiments on the Raspberry Pi.
