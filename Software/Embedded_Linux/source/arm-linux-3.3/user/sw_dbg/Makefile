#CROSS_COMPILE=/usr/src/arm-linux-3.3/arm-none-linux-gnueabi-4.4.0_linux-3.3/bin/arm-none-linux-gnueabi-
CROSS_COMPILE=/usr/src/arm-linux-3.3/toolchain_gnueabi-4.4.0_ARMv5TE/usr/bin/arm-unknown-linux-uclibcgnueabi-
AS	=$(CROSS_COMPILE)as
LD	=$(CROSS_COMPILE)ld
CC	=$(CROSS_COMPILE)gcc
CPP	=$(CC) -E
AR	=$(CROSS_COMPILE)ar
NM	=$(CROSS_COMPILE)nm
STRIP	=$(CROSS_COMPILE)strip
OBJCOPY =$(CROSS_COMPILE)objcopy
OBJDUMP =$(CROSS_COMPILE)objdump
RANLIB	=$(CROSS_COMPILE)ranlib

PWD := $(shell pwd)

DEPFLAGS=-MMD

CFLAGS = -O2 -Wall $(DEPFLAGS)
LDFLAGS = -lpthread #-static
LIBS =

PROGS = sw_dbg

all:: $(PROGS)

%: %.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)
	@$(NM) -n $@ | grep -v '\( [aUw] \)\|\(__crc_\)\|\( \$[adt]\)' > $@.map
	@$(STRIP) $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

sw_dbg: sw_dbg.o config.o

clean:
	@rm -f $(PROGS)
	@find . \( -name '*.[oas]' -o -name '*.ko' -o -name '.*.cmd' -o -name '*~' \
	-o -name '.*.d' -o -name '.*.tmp' -o -name '*.mod.c' -o -name '*.d' -o -name '*.map' \) \
	-type f -print | xargs rm -f

# Include dependancies
-include *.d
