DEBUG_MA = N

ifeq ($(DEBUG_MA), Y)
	CFLAGS += -g -DDEBUG
	CC = gcc
	STRIP =
else
	CFLAGS += -O2
	CC = /usr/src/arm-linux-3.3/toolchain_gnueabi-4.4.0_ARMv5TE/usr/bin/arm-unknown-linux-uclibcgnueabi-gcc
	STRIP = /usr/src/arm-linux-3.3/toolchain_gnueabi-4.4.0_ARMv5TE/usr/bin/arm-unknown-linux-uclibcgnueabi-strip
endif

PROGS = i2c_access

CFLAGS += -Wall -std=c99

ifeq ($(DEBUG_MA), Y)
$(PROGS): i2c_access.o 
else
$(PROGS): i2c_access.o 
	$(CC) -o $(PROGS) i2c_access.o
	$(STRIP) $(PROGS)
endif

%: %.o 
	$(CC) $(LDFLAGS) -o $@ $^ 

%.o: %.c Makefile
	$(CC) $(CFLAGS) -c -o $@ $<


clean:
	@rm -f $(PROGS)
	@find . \( -name '*.[oas]' -o -name '*.ko' -o -name '.*.cmd' -o -name '*~' \
        -o -name '.*.d' -o -name '.*.tmp' -o -name '*.mod.c' \) \
        -type f -print | xargs rm -f

