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

ES8328_APP := es8328_ioc
ES8328_APPOBJS := es8328_ioc.o

.PHONY: all

all: $(ES8328_APP)

$(ES8328_APPOBJS): %.o:%.c
	@echo [Complier... $^]
	@$(CC) $(CFLAGS) -c $< -o $@

$(ES8328_APP): $(ES8328_APPOBJS)
	@echo [Link... $@]
	@$(CC) -o $@ $(ES8328_APPOBJS:.o=.c) $(CFLAGS) $(LDFLAGS)


#########################################################################

clean:
	rm -f $(ES8328_APP) $(ES8328_APPOBJS)
