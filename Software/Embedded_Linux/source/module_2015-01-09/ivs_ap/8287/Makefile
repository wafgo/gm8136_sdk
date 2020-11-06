sinclude $(PWD)/../module_conf
AR=$(CROSS_COMPILE)ar
LD=$(CROSS_COMPILE)ld
CC=$(CROSS_COMPILE)gcc
CFLAGS= -Wall -march=armv4 -mtune=arm9tdmi -O3

SDK_MODULE_SRC := $(SDK_SRC)module/

SDL_CFLAGS = -I./

OBJDEC = ivs_ap.o
all:	$(OBJDEC)
	$(CC) -lc -lm $(SDL_CFLAGS) $(CFLAGS) $(OBJDEC) -o ivs_ap

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f ivs_ap  
	rm -f *.o
