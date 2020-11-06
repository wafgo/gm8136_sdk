sinclude $(PWD)/../module_conf

ifeq ($(obj),)
obj=$(PWD)
endif

ifndef SDK_MODULE_SRC
VERS2628 = $(findstring arm-linux-2.6.28,$(obj))
VERS3300 = $(findstring arm-linux-3.3,$(obj))

ifneq ($(VERS3300),)
SDK_SRC := /usr/src/arm-linux-3.3/
SDK_MODULE_SRC = $(SDK_SRC)module/
SDK_MODULE_INC = $(SDK_MODULE_SRC)include/security/
CROSS_COMPILE=arm-unknown-linux-uclibcgnueabi-

else ifneq ($(VERS2628),)
SDK_SRC := /usr/src/arm-linux-2.6.28/
SDK_MODULE_SRC = $(SDK_SRC)module/
SDK_MODULE_INC = $(SDK_MODULE_SRC)include/security/
CROSS_COMPILE=arm-none-linux-gnueabi-

else
SDK_SRC := /usr/src/arm-linux-2.6/
SDK_MODULE_SRC = $(SDK_SRC)module/
SDK_MODULE_INC = $(SDK_MODULE_SRC)AES_DES/
CROSS_COMPILE=arm-linux-
endif

endif

AR=$(CROSS_COMPILE)ar
LD=$(CROSS_COMPILE)ld
CC=$(CROSS_COMPILE)gcc

EXTRA_CFLAGS += -Wall -g -I$(SDK_MODULE_INC)  -I./

OBJTEST = security_test.o

all:  test

test:	$(OBJTEST)
	$(CC) -lc -lm $(OBJTEST) -o s_test

%.o: %.c
	$(CC) $(EXTRA_CFLAGS) -c -o $@ $<


Makefile.depend depend:
	$(CC) $(CFLAGS) -MM -MG *.c > Makefile.depend

clean:
	@rm -f *.o s_test

