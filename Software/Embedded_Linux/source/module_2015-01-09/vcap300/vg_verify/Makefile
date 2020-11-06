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

VG_MODULE_INC = $(SDK_MODULE_INCDIR)/videograph.include

include $(LINUX_SRC)/.config

EXTRA_CFLAGS += -Wall -g -I$(SDK_MODULE_INCDIR) -I$(VG_MODULE_INC) -I$(VG_MODULE_INC)/em

ifeq ($(CONFIG_PLATFORM_A369),y)
EXTRA_CFLAGS += -DPLATFORM_A369=1
endif

ifeq ($(CONFIG_PLATFORM_GM8210),y)
EXTRA_CFLAGS += -DPLATFORM_GM8210=1
endif

ifeq ($(CONFIG_PLATFORM_GM8287),y)
EXTRA_CFLAGS += -DPLATFORM_GM8287=1
endif

ifeq ($(CONFIG_PLATFORM_GM8139),y)
EXTRA_CFLAGS += -DPLATFORM_GM8139=1
endif

ifeq ($(CONFIG_PLATFORM_GM8136),y)
EXTRA_CFLAGS += -DPLATFORM_GM8136=1
endif

#########################################################################

VG_VERIFY = vg_verify

VG_VERIFY_OBJS := vg_verify.o config.o

.PHONY: all

all: $(VG_VERIFY)

$(VG_VERIFY_OBJS): %.o:%.c
	@echo [Complier... $^]
	@$(CC) $(EXTRA_CFLAGS) -c $< -o $@

$(VG_VERIFY): $(VG_VERIFY_OBJS)
	@echo [Link... $@]
	@$(CC) -o $@ $(VG_VERIFY_OBJS:.o=.c) $(EXTRA_CFLAGS)

#########################################################################

clean:
	rm -f $(VG_VERIFY) $(VG_VERIFY_OBJS)
