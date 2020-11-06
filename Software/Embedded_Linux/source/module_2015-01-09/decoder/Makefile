ifeq ($(KERNELRELEASE),)
PWD := $(shell pwd)
sinclude $(PWD)/../module_conf
endif

MODULE := decoder

ifeq ($(obj),)
obj=$(PWD)
endif

include $(obj)/config.mak

SDK_MODULE_SRC := $(SDK_SRC)module/
EXTRA_CFLAGS+=-DFPGA8210 -DLINUX
EXTRA_CFLAGS+=-I$(SDK_MODULE_SRC)/include/frammap -I$(SDK_MODULE_SRC)/include/ftmcp100
EXTRA_CFLAGS+=-I$(SDK_MODULE_SRC)/include/videograph.include/log -I$(SDK_MODULE_SRC)/include/videograph.include/em
#EXTRA_CFLAGS+=-Werror -g -I$(LINUX_INC) -I$(VG_MODULE_INC) -I$(VG_MODULE_INC)/log -I$(VG_MODULE_INC)/em -I$(CUS_INC) -I$(CUR_PATH)
EXTRA_CFLAGS+=-DHANDLE_PUTJOB_FAIL

STRIPFLAGS = -g --strip-unneeded
STRIP = $(CROSS_COMPILE)strip

$(MODULE)-objs += header.o bitstream.o decoder_vg.o
module-y += decoder.o

obj-m += $(module-y)
obj-m := $(sort $(obj-m))

PWD := $(shell pwd)

modules:
	$(MAKE) -C $(LINUX_SRC) SDK_MODULE_SRC=$(SDK_MODULE_SRC) SDK_MODULE_INC=$(SDK_MODULE_INC) LINUX_SRC=$(LINUX_SRC) M=$(PWD) modules
	$(STRIP) $(STRIPFLAGS) $(MODULE).ko

.PHONEY: clean
clean:
	$(MAKE) -C $(LINUX_SRC) M=$(PWD) clean

install:
	cp *.ko $(VG_MODULE_SRC)/bin

.PHONEY: all
all: modules

copy:
	cp *.ko /data/test_bench/platform8210/module/
