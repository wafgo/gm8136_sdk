#########################################################################
#                                                                       #
#       Copyright (C) 2013-2014 Grain Media, Corp.                      #
#       All Rights Reserved.                                            #
#                                                                       #
#########################################################################
PWD := $(shell pwd)
sinclude $(PWD)../../../../module_conf

CC    := $(CROSS_COMPILE)gcc
AR    := $(CROSS_COMPILE)ar
LD    := $(CROSS_COMPILE)ld
AS    := $(CROSS_COMPILE)as
STRIP := $(CROSS_COMPILE)strip

STRIPFLAGS = -g --strip-unneeded

DH9901_INCDIR = ../dh9901_lib
DH9901_LIBS = $(DH9901_INCDIR)/dh9901_a.lib

EXTRA_CFLAGS += -Wall -g -I$(DH9901_INCDIR)
LDFLAGS += -static $(DH9901_LIBS) -lpthread

#########################################################################

DH9901_DAEMON = dh9901_daemon

DH9901_DAEMON_OBJS := config.o dh9901_daemon.o

.PHONY: all

all: $(DH9901_DAEMON)

$(DH9901_DAEMON_OBJS): %.o:%.c
	@echo [Complier... $^]
	@$(CC) $(EXTRA_CFLAGS) -c $< -o $@

$(DH9901_DAEMON): $(DH9901_DAEMON_OBJS)
	@echo [Link... $@]
	@$(CC) -o $@ $(DH9901_DAEMON_OBJS:.o=.c) $(EXTRA_CFLAGS) $(LDFLAGS)

#########################################################################

clean:
	@rm -f $(DH9901_DAEMON) $(DH9901_DAEMON_OBJS)
