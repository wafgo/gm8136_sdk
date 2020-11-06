#########################################################################
#                                                                       #
#       Copyright (C) 2013-2014 Grain Media, Corp.                      #
#       All Rights Reserved.                                            #
#                                                                       #
#########################################################################

PWD := $(shell pwd)
sinclude $(PWD)/../../../../module_conf

SDK_MODULE_SRC := $(SDK_SRC)module
FE_DEC_INCDIR  := $(SDK_MODULE_SRC)/include/front_end/decoder

CC := $(CROSS_COMPILE)gcc
AR := $(CROSS_COMPILE)ar
LD := $(CROSS_COMPILE)ld
AS := $(CROSS_COMPILE)as
STRIP := $(CROSS_COMPILE)strip

EXTRA_CFLAGS+=-O2 -Wall
CFLAGS  := $(CFLAGS) -I$(FE_DEC_INCDIR)/
LDFLAGS := $(EXTRA_FLAGS) $(LDFLAGS)

#########################################################################

TW2865_APP := tw2865_ioc
TW2865_APPOBJS := tw2865_ioc.o

TW2868_APP := tw2868_ioc
TW2868_APPOBJS := tw2868_ioc.o

TW2968_APP := tw2968_ioc
TW2968_APPOBJS := tw2968_ioc.o

TW2964_APP := tw2964_ioc
TW2964_APPOBJS := tw2964_ioc.o

.PHONY: all

all: $(TW2865_APP) $(TW2868_APP) $(TW2968_APP) $(TW2964_APP)

$(TW2865_APPOBJS): %.o:%.c
	@echo [Complier... $^]
	@$(CC) $(CFLAGS) -c $< -o $@

$(TW2865_APP): $(TW2865_APPOBJS)
	@echo [Link... $@]
	@$(CC) -o $@ $(TW2865_APPOBJS:.o=.c) $(CFLAGS) $(LDFLAGS)

$(TW2868_APPOBJS): %.o:%.c
	@echo [Complier... $^]
	@$(CC) $(CFLAGS) -c $< -o $@

$(TW2868_APP): $(TW2868_APPOBJS)
	@echo [Link... $@]
	@$(CC) -o $@ $(TW2868_APPOBJS:.o=.c) $(CFLAGS) $(LDFLAGS)

$(TW2968_APPOBJS): %.o:%.c
	@echo [Complier... $^]
	@$(CC) $(CFLAGS) -c $< -o $@

$(TW2968_APP): $(TW2968_APPOBJS)
	@echo [Link... $@]
	@$(CC) -o $@ $(TW2968_APPOBJS:.o=.c) $(CFLAGS) $(LDFLAGS)

$(TW2964_APPOBJS): %.o:%.c
	@echo [Complier... $^]
	@$(CC) $(CFLAGS) -c $< -o $@

$(TW2964_APP): $(TW2964_APPOBJS)
	@echo [Link... $@]
	@$(CC) -o $@ $(TW2964_APPOBJS:.o=.c) $(CFLAGS) $(LDFLAGS)

#########################################################################

clean:
	rm -f $(TW2865_APP) $(TW2865_APPOBJS) $(TW2868_APP) $(TW2868_APPOBJS) $(TW2968_APP) $(TW2968_APPOBJS) $(TW2964_APP) $(TW2964_APPOBJS)
