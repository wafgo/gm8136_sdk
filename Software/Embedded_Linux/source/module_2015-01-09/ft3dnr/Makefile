sinclude $(PWD)/../module_conf

VIDEOGRAPH_INC=1
MODULE = ft3dnr

SDK_MODULE_SRC := $(SDK_SRC)module/
STRIPFLAGS = -g --strip-unneeded
STRIP = $(CROSS_COMPILE)strip

ifneq ($(KERNELRELEASE),)

include $(srctree)/.config

ft3dnr-objs-y += gm8139.o

EXTRA_CFLAGS+=$(PLATFORM_CFLAGS)

ifdef VIDEOGRAPH_INC
EXTRA_CFLAGS+=-I$(SDK_MODULE_SRC)/include/videograph.include/log -I$(SDK_MODULE_SRC)/include/videograph.include/em
EXTRA_CFLAGS+=-DVIDEOGRAPH_INC=1
endif

EXTRA_CFLAGS+=-I$(SDK_MODULE_SRC)/include/frammap -I$(SDK_MODULE_SRC)/include/ft3dnr

ft3dnr-objs := ft3dnr_drv.o ft3dnr_hw.o ft3dnr_vg.o api_if.o ft3dnr_mem.o ft3dnr_ctrl.o ft3dnr_mrnr.o ft3dnr_tmnr.o ft3dnr_dma.o $(ft3dnr-objs-y)

modules-yy += ft3dnr.o

obj-m += $(modules-yy)
obj-m := $(sort $(obj-m))

else

PWD := $(shell pwd)

modules:
	$(MAKE) -C $(LINUX_SRC) M=$(PWD) VIDEOGRAPH_INC=1 modules
	$(STRIP) $(STRIPFLAGS) $(MODULE).ko

clean:
	$(MAKE) -C $(LINUX_SRC) M=$(PWD) clean
endif





