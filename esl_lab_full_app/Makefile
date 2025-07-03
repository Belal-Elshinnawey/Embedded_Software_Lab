CC       = gcc

CFLAGS   = -Wall -Ofast \
           -Iapp/include \
           -Irt_utils/include \
           -IImage_Processor/include \
           -Ipitch_model/include \
           -Iyaw_model/include \
           -Itime_profiling/include \
           $(shell pkg-config --cflags gstreamer-1.0 gstreamer-app-1.0)

-include config.mk

PROFILE ?= 0
DEBUG_VIDEO ?= 0

ifeq ($(DEBUG_VIDEO), 1)
    CFLAGS += -DDEBUG_VIDEO
endif

ifeq ($(PROFILE), 1)
    CFLAGS += -DMEASURE_THREADS
endif

LDLIBS   = -lpthread -lm -lreadline \
           $(shell pkg-config --libs gstreamer-1.0 gstreamer-app-1.0)

LDLIBS  += $(shell pkg-config --libs sdl2) 

SRC_DIRS = src \
           rt_utils/src \
           pitch_model/src \
           yaw_model/src \
           Image_Processor/src \
           time_profiling/src \
           app/src

PLATFORM ?= RPI

ifeq ($(PLATFORM), RPI)
    CFLAGS   += -DPLATFORM_RPI
    LDLIBS   += -lgpiod
else ifeq ($(PLATFORM), DE10)
    CFLAGS   += -DPLATFORM_DE10 -Ide10_conf/include
else
    $(error Unsupported PLATFORM: $(PLATFORM))
endif

SRCS     = $(foreach dir,$(SRC_DIRS),$(wildcard $(dir)/*.c))
OBJS     = $(SRCS:.c=.o)

TARGET   = start_app

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
