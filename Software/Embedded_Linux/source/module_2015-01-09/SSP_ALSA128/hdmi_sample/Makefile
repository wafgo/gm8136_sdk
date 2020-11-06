CC = /usr/src/arm-linux-3.3/toolchain_gnueabi-4.4.0_ARMv5TE/usr/bin/arm-unknown-linux-uclibcgnueabi-gcc

CFLAGS = -O2 -Wall
LDFLAGS = -Wl,-s

LIBS=

PROGS = hdmi_play

all:: $(PROGS)

%: %.o
	$(CC) $(LDFLAGS) -lpthread -o $@ $^ $(LIBS)
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<


hdmi_play: hdmi_play.o

clean:
	@rm -f $(PROGS)
	@find . \( -name '*.[oas]' -o -name '*.ko' -o -name '.*.cmd' -o -name '*~' \
	-o -name '.*.d' -o -name '.*.tmp' -o -name '*.mod.c' \) \
	-type f -print | xargs rm -f
