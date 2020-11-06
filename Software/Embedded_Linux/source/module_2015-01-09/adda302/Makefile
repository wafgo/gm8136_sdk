sinclude $(PWD)/../module_conf

MODULE = adda302

SDK_MODULE_SRC := $(SDK_SRC)module/
STRIPFLAGS = -g --strip-unneeded
STRIP = $(CROSS_COMPILE)strip

ifneq ($(KERNELRELEASE),)

include $(srctree)/.config

ifeq ($(CONFIG_PLATFORM_GM8139),y)
adda302-objs-y += gm8139.o
endif

ifeq ($(CONFIG_PLATFORM_GM8136),y)
adda302-objs-y += gm8136.o
endif

EXTRA_CFLAGS+=$(PLATFORM_CFLAGS)
EXTRA_CFLAGS+=-I$(SDK_MODULE_SRC)/include/adda302
EXTRA_CFLAGS+=-I$(SDK_MODULE_SRC)/front_end

adda302-objs := adda302_drv.o adda302_proc.o $(adda302-objs-y)

modules-yy += adda302.o

obj-m += $(modules-yy)
obj-m := $(sort $(obj-m))

else

PWD := $(shell pwd)

modules:
	$(MAKE) -C $(LINUX_SRC) M=$(PWD)
	$(STRIP) $(STRIPFLAGS) $(MODULE).ko

clean:
	$(MAKE) -C $(LINUX_SRC) M=$(PWD) clean
endif





