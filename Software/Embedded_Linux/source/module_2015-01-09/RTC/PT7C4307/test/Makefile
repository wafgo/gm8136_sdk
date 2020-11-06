#########################################################################
#                                                                       #
#       Copyright (C) 2013-2014 Grain Media, Corp.                      #
#       All Rights Reserved.                                            #
#                                                                       #
#########################################################################

PWD := $(shell pwd)
sinclude $(PWD)/../../../module_conf

CC := $(CROSS_COMPILE)gcc
AR := $(CROSS_COMPILE)ar
LD := $(CROSS_COMPILE)ld
AS := $(CROSS_COMPILE)as
STRIP := $(CROSS_COMPILE)strip

EXTRA_CFLAGS+=-O2 -Wall
CFLAGS  := $(CFLAGS)
LDFLAGS := $(EXTRA_FLAGS) $(LDFLAGS)

#########################################################################

RTCTEST_APP := rtctest

RTCTEST_APPOBJS := rtctest.o

.PHONY: all

all: $(RTCTEST_APP)

$(RTCTEST_APPOBJS): %.o:%.c
	@echo [Complier... $^]
	@$(CC) $(CFLAGS) -c $< -o $@

$(RTCTEST_APP): $(RTCTEST_APPOBJS)
	@echo [Link... $@]
	@$(CC) -o $@ $(RTCTEST_APPOBJS:.o=.c) $(CFLAGS) $(LDFLAGS)

#########################################################################

clean:
	rm -f $(RTCTEST_APP) $(RTCTEST_APPOBJS)
