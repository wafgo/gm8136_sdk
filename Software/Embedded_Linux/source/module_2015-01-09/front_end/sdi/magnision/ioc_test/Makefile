#########################################################################
#                                                                       #
#       Copyright (C) 2013-2014 Grain Media, Corp.                      #
#       All Rights Reserved.                                            #
#                                                                       #
#########################################################################

PWD := $(shell pwd)
sinclude $(PWD)/../../../../module_conf

SDK_MODULE_SRC := $(SDK_SRC)module
FE_DEC_INCDIR  := $(SDK_MODULE_SRC)/include/front_end/sdi

CC := $(CROSS_COMPILE)gcc
AR := $(CROSS_COMPILE)ar
LD := $(CROSS_COMPILE)ld
AS := $(CROSS_COMPILE)as
STRIP := $(CROSS_COMPILE)strip

EXTRA_CFLAGS+=-O2 -Wall
CFLAGS  := $(CFLAGS) -I$(FE_DEC_INCDIR)/
LDFLAGS := $(EXTRA_FLAGS) $(LDFLAGS)

#########################################################################

MV4101_APP := mv4101_ioc
MV4101_APPOBJS := mv4101_ioc.o

.PHONY: all

all: $(MV4101_APP)

$(MV4101_APPOBJS): %.o:%.c
	@echo [Complier... $^]
	@$(CC) $(CFLAGS) -c $< -o $@

$(MV4101_APP): $(MV4101_APPOBJS)
	@echo [Link... $@]
	@$(CC) -o $@ $(MV4101_APPOBJS:.o=.c) $(CFLAGS) $(LDFLAGS)

#########################################################################

clean:
	rm -f $(MV4101_APP) $(MV4101_APPOBJS)
