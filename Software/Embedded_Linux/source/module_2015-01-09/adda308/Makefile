sinclude $(PWD)/../module_conf

MODULE = adda308

SDK_MODULE_SRC := $(SDK_SRC)module/
STRIPFLAGS = -g --strip-unneeded
STRIP = $(CROSS_COMPILE)strip

ifneq ($(KERNELRELEASE),)

include $(srctree)/.config

adda308-objs-y += gm8136.o

EXTRA_CFLAGS+=$(PLATFORM_CFLAGS)
EXTRA_CFLAGS+=-I$(SDK_MODULE_SRC)/include/adda308
EXTRA_CFLAGS+=-I$(SDK_MODULE_SRC)/front_end

adda308-objs := adda308_drv.o adda308_proc.o $(adda308-objs-y)

modules-yy += adda308.o

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





