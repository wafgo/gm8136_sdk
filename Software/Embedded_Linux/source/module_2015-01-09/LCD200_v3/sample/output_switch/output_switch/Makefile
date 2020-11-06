GMLIB=/usr/src/gm_graph/gm_lib

sinclude $(GMLIB)/gmlib.mak
CROSS_COMPILE=/usr/src/arm-linux-3.3/toolchain_gnueabi-4.4.0_ARMv5TE/usr/bin/arm-unknown-linux-uclibcgnueabi-
CC=$(CROSS_COMPILE)gcc
CPP=$(CC) -E
LD=$(CROSS_COMPILE)ld
AS=$(CROSS_COMPILE)as
MAKE=make

LDFLAGS += -L$(GMLIB)/lib -lpthread -lgm
CFLAGS += -Wall -I$(GMLIB)/inc

TARGETS := perf_4d1

all: $(TARGETS)

$(TARGETS): %: %.c Makefile $(GMLIB)/inc/gmlib.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<
	cp $@ /tmp1

clean:
	rm -f $(TARGETS)
