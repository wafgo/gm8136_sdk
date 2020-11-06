PWD := $(shell pwd)
sinclude $(PWD)/../../../module_conf

STRIPFLAGS = -g --strip-unneeded
STRIP = $(CROSS_COMPILE)strip

SDK_MODULE_SRC := $(SDK_SRC)module
KERNELDIR := $(LINUX_SRC)
FE_TVI_INCDIR := $(SDK_MODULE_SRC)/include/front_end/hdtvi
FE_SDD1_INCDIR := $(SDK_MODULE_SRC)/include/front_end/decoder

ifneq ($(KERNELRELEASE),)

include $(srctree)/.config

EXTRA_CFLAGS += -Wall -g -I$(obj)/../../ -I$(FE_TVI_INCDIR)/ -I$(FE_SDD1_INCDIR)/

tp2802_tw2964-objs := tw2964_lib.o tw2964_dev.o tp2802_lib.o tp2802_dev.o tp2802_tw2964_hybrid_drv.o
obj-m += tp2802_tw2964.o

obj-m += $(modules-y)
obj-m := $(sort $(obj-m))

else

all modules:
	$(MAKE) -C $(KERNELDIR) SDK_MODULE_SRC=$(SDK_MODULE_SRC) M=$(PWD) modules
	@$(STRIP) $(STRIPFLAGS) *.ko

module_strip:
	@$(STRIP) $(STRIPFLAGS) *.ko

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

endif
