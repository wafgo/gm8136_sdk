PWD := $(shell pwd)
sinclude $(PWD)/../../module_conf
CC = $(CROSS_COMPILE)gcc
STRIP = $(CROSS_COMPILE)strip

CFLAGS = -O2 -Wall -I../../include/
LDFLAGS = -Wl,-s
STRIPFLAGS = -g --strip-unneeded

LIBS=

PROGS = dma_test

all::$(PROGS)

%: %.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)
	$(STRIP) $(STRIPFLAGS) $(PROGS)
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

dma_test: dma_test.o

copy:
	cp -f $(PROGS) ../../../target/rootfs-cpio/bin/

.PHONY:clean
clean:
	@rm -f $(PROGS)
	@find . \( -name '*.[oas]' -o -name '*.ko' -o -name '.*.cmd' -o -name '*~' \
	-o -name '.*.d' -o -name '.*.tmp' -o -name '*.mod.c' \) \
	-type f -print | xargs rm -f
