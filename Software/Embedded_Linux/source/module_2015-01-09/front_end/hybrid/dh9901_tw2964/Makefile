PWD := $(shell pwd)
sinclude $(PWD)/../../../module_conf

STRIPFLAGS = -g --strip-unneeded
STRIP = $(CROSS_COMPILE)strip

SDK_MODULE_SRC := $(SDK_SRC)module
KERNELDIR := $(LINUX_SRC)
FE_HDCVI_INCDIR := $(SDK_MODULE_SRC)/include/front_end/hdcvi
FE_HDCVI_LDDIR := $(SDK_MODULE_SRC)/front_end/hdcvi/dahua/dh9901_lib
FE_SDD1_INCDIR := $(SDK_MODULE_SRC)/include/front_end/decoder

ifneq ($(KERNELRELEASE),)

include $(srctree)/.config

EXTRA_CFLAGS += -Wall -g -I$(obj)/../../ -I$(FE_HDCVI_INCDIR)/ -I$(FE_HDCVI_LDDIR)/ -I$(FE_SDD1_INCDIR)/

dh9901_tw2964-objs := tw2964_lib.o tw2964_dev.o dh9901_dev.o dh9901_tw2964_hybrid_drv.o
obj-m += dh9901_tw2964.o

obj-m += $(modules-y)
obj-m := $(sort $(obj-m))

else

all modules:
	$(MAKE) -C $(KERNELDIR) SDK_MODULE_SRC=$(SDK_MODULE_SRC) M=$(PWD) modules
	@cp $(FE_HDCVI_LDDIR)/dh9901_ko.lib $(PWD)/dh9901.ko
	@$(STRIP) $(STRIPFLAGS) *.ko

module_strip:
	@cp $(FE_HDCVI_LDDIR)/dh9901_ko.lib $(PWD)/dh9901.ko
	@$(STRIP) $(STRIPFLAGS) *.ko

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

endif
