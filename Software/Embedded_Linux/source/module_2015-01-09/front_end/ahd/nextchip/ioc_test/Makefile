#########################################################################
#                                                                       #
#       Copyright (C) 2013-2014 Grain Media, Corp.                      #
#       All Rights Reserved.                                            #
#                                                                       #
#########################################################################

PWD := $(shell pwd)
sinclude $(PWD)/../../../../module_conf

SDK_MODULE_SRC := $(SDK_SRC)module
FE_AHD_INCDIR  := $(SDK_MODULE_SRC)/include/front_end/ahd

CC := $(CROSS_COMPILE)gcc
AR := $(CROSS_COMPILE)ar
LD := $(CROSS_COMPILE)ld
AS := $(CROSS_COMPILE)as
STRIP := $(CROSS_COMPILE)strip

EXTRA_CFLAGS+=-O2 -Wall
CFLAGS  := $(CFLAGS) -I$(FE_AHD_INCDIR)/
LDFLAGS := $(EXTRA_FLAGS) $(LDFLAGS)

#########################################################################

NVP6114_APP := nvp6114_ioc
NVP6114_APPOBJS := nvp6114_ioc.o

NVP6124_APP := nvp6124_ioc
NVP6124_APPOBJS := nvp6124_ioc.o

.PHONY: all

all: $(NVP6114_APP) $(NVP6124_APP)

$(NVP6114_APPOBJS): %.o:%.c
	@echo [Complier... $^]
	@$(CC) $(CFLAGS) -c $< -o $@

$(NVP6114_APP): $(NVP6114_APPOBJS)
	@echo [Link... $@]
	@$(CC) -o $@ $(NVP6114_APPOBJS:.o=.c) $(CFLAGS) $(LDFLAGS)

$(NVP6124_APPOBJS): %.o:%.c
	@echo [Complier... $^]
	@$(CC) $(CFLAGS) -c $< -o $@

$(NVP6124_APP): $(NVP6124_APPOBJS)
	@echo [Link... $@]
	@$(CC) -o $@ $(NVP6124_APPOBJS:.o=.c) $(CFLAGS) $(LDFLAGS)

#########################################################################

clean:
	rm -f $(NVP6114_APP) $(NVP6114_APPOBJS) $(NVP6124_APP) $(NVP6124_APPOBJS)
