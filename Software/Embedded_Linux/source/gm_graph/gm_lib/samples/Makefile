CUR_PATH := $(shell pwd)

ifeq ($(shell find $(CUR_PATH)/../ -name gmlib.mak),)
    sinclude /usr/src/arm-linux-3.3/linux-3.3-fa/cross_compiler_def
else
    sinclude $(CUR_PATH)/../gmlib.mak
endif

CC=$(CROSS_COMPILE)gcc
STRIP=$(CROSS_COMPILE)strip
CPP=$(CC) -E
LD=$(CROSS_COMPILE)ld
AS=$(CROSS_COMPILE)as
MAKE=make

LIB=../lib
LDFLAGS += -L$(LIB) -lpthread -lgm
CFLAGS += -Wall -I../inc

TARGETS := \
           display_with_encode \
           liveview_with_clearwin \
           liveview_with_pip \
           encode_capture_substream \
           encode_with_deinterlace \
           encode_with_osd \
           encode_with_roi \
           encode_force_keyframe \
           encode_with_snapshot \
           encode_with_watermark_and_vui \
           encode_with_capture_motion_detection \
           encode_with_capture_motion_detection2 \
           encode_with_capture_tamper_detection \
           encode_with_capture_tamper_detection2 \
           encode_update_notification \
           encode_with_advance_feature \
           encode_with_getraw \
           encode_with_getraw2 \
           encode_with_eptz \
           encode_with_av_sync \
           audio_record \
           audio_playback \
           audio_livesound \


all: $(TARGETS)

$(TARGETS): %: %.c Makefile ../inc/gmlib.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<
	$(STRIP) $@
	cp $@ /tmp

clean:
	rm -f $(TARGETS)
