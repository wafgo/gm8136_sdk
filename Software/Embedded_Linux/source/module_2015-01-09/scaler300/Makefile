sinclude $(PWD)/../module_conf

VIDEOGRAPH_INC=1
MODULE = fscaler300

SDK_MODULE_SRC := $(SDK_SRC)module/
STRIPFLAGS = -g --strip-unneeded
STRIP = $(CROSS_COMPILE)strip

ifneq ($(KERNELRELEASE),)

include $(srctree)/.config

ifeq ($(CONFIG_PLATFORM_GM8210),y)
CONFIG_FILE=config_8210
fscaler300-objs-y += gm8210.o
endif

ifeq ($(CONFIG_PLATFORM_GM8287),y)
CONFIG_FILE=config_8287
fscaler300-objs-y += gm8287.o
endif

ifeq ($(CONFIG_PLATFORM_GM8139),y)
CONFIG_FILE=config_8139
fscaler300-objs-y += gm8139.o
endif

ifeq ($(CONFIG_PLATFORM_GM8136),y)
CONFIG_FILE=config_8136
fscaler300-objs-y += gm8136.o
endif

include $(SDK_MODULE_SRC)/scaler300/$(CONFIG_FILE)
include $(SDK_MODULE_SRC)/scaler300/Makefile.rules

EXTRA_CFLAGS+=$(PLATFORM_CFLAGS)

ifdef VIDEOGRAPH_INC
EXTRA_CFLAGS+=-I$(SDK_MODULE_SRC)/include/videograph.include/log -I$(SDK_MODULE_SRC)/include/videograph.include/em
EXTRA_CFLAGS+=-DVIDEOGRAPH_INC=1
endif

EXTRA_CFLAGS+=-I$(SDK_MODULE_SRC)/include -I$(SDK_MODULE_SRC)/include/frammap -I$(SDK_MODULE_SRC)/include/scaler300 -I$(SDK_MODULE_SRC)/include/osd_dispatch
EXTRA_CFLAGS+=-I$(SDK_MODULE_SRC)/include/cpu_comm

fscaler300-objs := fscaler300_drv.o fscaler300_hw.o fscaler300_vg.o api_if.o fscaler300_osd.o fscaler300_fcs.o fscaler300_dn.o fscaler300_sharpness.o fscaler300_smooth.o fscaler300_dma.o fscaler300_mem.o scaler_trans.o $(fscaler300-objs-y)

modules-yy += fscaler300.o

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





