sinclude $(PWD)/../module_conf

ifeq ($(obj),)
obj=$(PWD)
endif

ifndef SDK_MODULE_SRC
VERS2614 = $(findstring arm-linux-2.6,$(obj))
VERS2628 = $(findstring arm-linux-2.6.28,$(obj))
VERS3300 = $(findstring arm-linux-3.3,$(obj))
CROSS_COMPILE=/usr/src/arm-linux-3.3/toolchain_gnueabi-4.4.0_ARMv5TE/usr/bin/arm-unknown-linux-uclibcgnueabi-

ifneq ($(VERS3300),)
SDK_SRC := /usr/src/arm-linux-3.3/
SDK_MODULE_SRC = $(SDK_SRC)module/
SDK_MODULE_INC = $(SDK_MODULE_SRC)include/


else ifneq ($(VERS2628),)
SDK_SRC := /usr/src/arm-linux-2.6.28/
SDK_MODULE_SRC = $(SDK_SRC)module/
SDK_MODULE_INC = $(SDK_MODULE_SRC)include/

else
SDK_SRC := /usr/src/arm-linux-2.6/
SDK_MODULE_SRC = $(SDK_SRC)module/
SDK_MODULE_INC = $(SDK_MODULE_SRC)

endif

endif




RAM_TARGET = $(SDK_SRC)target/rootfs-cpio/lib/modules

AR=$(CROSS_COMPILE)ar
LD=$(CROSS_COMPILE)ld
CC=$(CROSS_COMPILE)gcc


CFLAGS+=-I$(SDK_MODULE_INC)keyscan/  -I./

OBJTEST = keyscan_test.o

all:  test

test:	$(OBJTEST)
	$(CC) -lc -lm $(OBJTEST) -o keyscan_test
	cp keyscan_test $(RAM_TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
	

Makefile.depend depend:
	$(CC) $(CFLAGS) -MM -MG *.c > Makefile.depend
	
	 
clean:
	rm -f *.o
	rm -f keyscan_test

