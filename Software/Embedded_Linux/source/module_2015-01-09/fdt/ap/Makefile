sinclude $(PWD)/../../module_conf
AR=$(CROSS_COMPILE)ar
LD=$(CROSS_COMPILE)ld
CC=$(CROSS_COMPILE)gcc
CFLAGS= -Wall -march=armv4 -mtune=arm9tdmi -O3

SDK_MODULE_SRC := $(SDK_SRC)module/

SDL_CFLAGS = -I./

OBJDEC = fdt_ap.o
all:	$(OBJDEC)
	$(CC) -lc -lm $(SDL_CFLAGS) $(CFLAGS) $(OBJDEC) -o fdt_ap

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f fdt_ap
	rm -f *.o
