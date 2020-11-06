sinclude $(PWD)/../../module_conf
CC = $(CROSS_COMPILE)gcc
STRIP = $(CROSS_COMPILE)strip

CFLAGS = -O2 -Wall -I../../include/pwm/
LDFLAGS = -Wl,-s
STRIPFLAGS = -g --strip-unneeded

LIBS=

PROGS = pwm_test

all::$(PROGS)

%: %.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)
	$(STRIP) $(STRIPFLAGS) $(PROGS)
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

pwm_test: pwm_test.o

copy:
	cp -f $(PROGS) ../../../target/rootfs-cpio/bin/

clean:
	@rm -f $(PROGS)
	@find . \( -name '*.[oas]' -o -name '*.ko' -o -name '.*.cmd' -o -name '*~' \
	-o -name '.*.d' -o -name '.*.tmp' -o -name '*.mod.c' \) \
	-type f -print | xargs rm -f
