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

SDK_MODULE_SRC := $(SDK_SRC)module
SDK_MODULE_INCDIR := $(SDK_MODULE_SRC)/include

SECURITY_INC = $(SDK_MODULE_INCDIR)/security

EXTRA_CFLAGS += -Wall -g -I$(SDK_MODULE_INCDIR) -I$(SECURITY_INC)

#########################################################################

SECURITY_VERIFY = s_verify

SECURITY_VERIFY_OBJS := s_verify.o

.PHONY: all

all: $(SECURITY_VERIFY)

$(SECURITY_VERIFY_OBJS): %.o:%.c
	@echo [Complier... $^]
	@$(CC) $(EXTRA_CFLAGS) -c $< -o $@

$(SECURITY_VERIFY): $(SECURITY_VERIFY_OBJS)
	@echo [Link... $@]
	@$(CC) -o $@ $(SECURITY_VERIFY_OBJS:.o=.c) $(EXTRA_CFLAGS)

#########################################################################

clean:
	rm -f $(SECURITY_VERIFY) $(SECURITY_VERIFY_OBJS)
