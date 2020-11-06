PWD := $(shell pwd)
sinclude $(PWD)/../../module_conf
CC = $(CROSS_COMPILE)gcc
STRIP = $(CROSS_COMPILE)strip

CFLAGS = -O2 -Wall -I../../include/gpio_functions/
LDFLAGS = -Wl,-s
STRIPFLAGS = -g --strip-unneeded

LIBS=

PROGS = cpucomm_file

all::$(PROGS)

%: %.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)
	$(STRIP) $(STRIPFLAGS) $(PROGS)
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

ap_xmit: cpucomm_file.o

.PHONY:clean
clean:
	@rm -f $(PROGS)
	@find . \( -name '*.[oas]' -o -name '*.ko' -o -name '.*.cmd' -o -name '*~' \
	-o -name '.*.d' -o -name '.*.tmp' -o -name '*.mod.c' \) \
	-type f -print | xargs rm -f
