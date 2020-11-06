PWD := $(shell pwd)
sinclude $(PWD)/../../../module_conf

STRIPFLAGS = -g --strip-unneeded
STRIP = $(CROSS_COMPILE)strip

SDK_MODULE_SRC := $(SDK_SRC)module
KERNELDIR := $(LINUX_SRC)
FE_DEC_INCDIR := $(SDK_MODULE_SRC)/include/front_end/decoder

ifneq ($(KERNELRELEASE),)

include $(srctree)/.config

EXTRA_CFLAGS += -Wall -g -I$(obj)/../../ -I$(FE_DEC_INCDIR)/

rn6314-objs := rn6314_drv.o rn6314_lib.o
obj-m += rn6314.o

rn6318-objs := rn6318_drv.o rn6318_lib.o
obj-m += rn6318.o

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