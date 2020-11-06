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

NVP1918_APP := nvp1918_ioc
NVP1918_APPOBJS := nvp1918_ioc.o

NVP1918C_APP := nvp1918c_ioc
NVP1918C_APPOBJS := nvp1918c_ioc.o

NVP1118_APP := nvp1118_ioc
NVP1118_APPOBJS := nvp1118_ioc.o

NVP1104_APP := nvp1104_ioc
NVP1104_APPOBJS := nvp1104_ioc.o

NVP1914_APP := nvp1914_ioc
NVP1914_APPOBJS := nvp1914_ioc.o

NVP1914C_APP := nvp1914c_ioc
NVP1914C_APPOBJS := nvp1914c_ioc.o

.PHONY: all

all: $(NVP1918_APP) $(NVP1918C_APP) $(NVP1118_APP) $(NVP1104_APP) $(NVP1914_APP) $(NVP1914C_APP)

$(NVP1918_APPOBJS): %.o:%.c
	@echo [Complier... $^]
	@$(CC) $(CFLAGS) -c $< -o $@

$(NVP1918_APP): $(NVP1918_APPOBJS)
	@echo [Link... $@]
	@$(CC) -o $@ $(NVP1918_APPOBJS:.o=.c) $(CFLAGS) $(LDFLAGS)

$(NVP1918C_APPOBJS): %.o:%.c
	@echo [Complier... $^]
	@$(CC) $(CFLAGS) -c $< -o $@

$(NVP1918C_APP): $(NVP1918C_APPOBJS)
	@echo [Link... $@]
	@$(CC) -o $@ $(NVP1918C_APPOBJS:.o=.c) $(CFLAGS) $(LDFLAGS)

$(NVP1118_APPOBJS): %.o:%.c
	@echo [Complier... $^]
	@$(CC) $(CFLAGS) -c $< -o $@

$(NVP1118_APP): $(NVP1118_APPOBJS)
	@echo [Link... $@]
	@$(CC) -o $@ $(NVP1118_APPOBJS:.o=.c) $(CFLAGS) $(LDFLAGS)

$(NVP1104_APPOBJS): %.o:%.c
	@echo [Complier... $^]
	@$(CC) $(CFLAGS) -c $< -o $@

$(NVP1104_APP): $(NVP1104_APPOBJS)
	@echo [Link... $@]
	@$(CC) -o $@ $(NVP1104_APPOBJS:.o=.c) $(CFLAGS) $(LDFLAGS)

$(NVP1914_APPOBJS): %.o:%.c
	@echo [Complier... $^]
	@$(CC) $(CFLAGS) -c $< -o $@

$(NVP1914_APP): $(NVP1914_APPOBJS)
	@echo [Link... $@]
	@$(CC) -o $@ $(NVP1914_APPOBJS:.o=.c) $(CFLAGS) $(LDFLAGS)

$(NVP1914C_APPOBJS): %.o:%.c
	@echo [Complier... $^]
	@$(CC) $(CFLAGS) -c $< -o $@

$(NVP1914C_APP): $(NVP1914C_APPOBJS)
	@echo [Link... $@]
	@$(CC) -o $@ $(NVP1914C_APPOBJS:.o=.c) $(CFLAGS) $(LDFLAGS)

#########################################################################

clean:
	rm -f $(NVP1918_APP) $(NVP1918_APPOBJS) $(NVP1918C_APP) $(NVP1918C_APPOBJS)
	rm -f $(NVP1118_APP) $(NVP1118_APPOBJS) $(NVP1104_APP) $(NVP1104_APPOBJS)
	rm -f $(NVP1914_APP) $(NVP1914_APPOBJS) $(NVP1914C_APP) $(NVP1914C_APPOBJS)
