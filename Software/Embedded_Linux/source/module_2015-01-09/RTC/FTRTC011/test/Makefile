#########################################################################
#                                                                       #
#       Copyright (C) 2013-2014 Grain Media, Corp.                      #
#       All Rights Reserved.                                            #
#                                                                       #
#########################################################################
PWD := $(shell pwd)
sinclude $(PWD)/../../module_conf

CC    := $(CROSS_COMPILE)gcc
AR    := $(CROSS_COMPILE)ar
LD    := $(CROSS_COMPILE)ld
AS    := $(CROSS_COMPILE)as
STRIP := $(CROSS_COMPILE)strip

STRIPFLAGS = -g --strip-unneeded

EXTRA_CFLAGS += -Wall -g

#########################################################################

RTC_TEST = rtc_test

RTC_TEST_OBJS := rtc_test.o

.PHONY: all

all: $(RTC_TEST)

$(RTC_TEST_OBJS): %.o:%.c
	@echo [Complier... $^]
	@$(CC) $(EXTRA_CFLAGS) -c $< -o $@

$(RTC_TEST): $(RTC_TEST_OBJS)
	@echo [Link... $@]
	@$(CC) -o $@ $(RTC_TEST_OBJS:.o=.c) $(EXTRA_CFLAGS)

#########################################################################

clean:
	rm -f $(RTC_TEST) $(RTC_TEST_OBJS)
