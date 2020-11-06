sinclude $(PWD)/../module_conf
PWD := $(shell pwd)

VIDEOGRAPH_INC=1

MODULE = ftdi220

SDK_MODULE_SRC := $(SDK_SRC)module/
STRIPFLAGS = -g --strip-unneeded
STRIP = $(CROSS_COMPILE)strip

ifdef VIDEOGRAPH_INC
EXTRA_CFLAGS+=-I$(SDK_MODULE_SRC)/include/videograph.include/log -I$(SDK_MODULE_SRC)/include/videograph.include/em
EXTRA_CFLAGS+=-DVIDEOGRAPH_INC=1
endif

EXTRA_CFLAGS+=-I$(SDK_MODULE_SRC)/include/frammap -I$(SDK_MODULE_SRC)/include/ftdi220

ifneq ($(KERNELRELEASE),)

ftdi220-objs-y$(CONFIG_PLATFORM_GM8210) += gm8210.o
ftdi220-objs-y$(CONFIG_PLATFORM_GM8287) += gm8287.o
ftdi220-objs-y$(CONFIG_PLATFORM_GM8139) += gm8139.o
ftdi220-objs-y$(CONFIG_PLATFORM_GM8136) += gm8136.o

ftdi220_vg-obj-y$(CONFIG_PLATFORM_GM8210) += ftdi220_vg_ch.o
ftdi220_vg-obj-y$(CONFIG_PLATFORM_GM8287) += ftdi220_vg_ch.o
ftdi220_vg-obj-y$(CONFIG_PLATFORM_GM8139) += ftdi220_vg_ch.o 
ftdi220_vg-obj-y$(CONFIG_PLATFORM_GM8136) += ftdi220_vg_ch.o


ifdef VIDEOGRAPH_INC
ftdi220-objs := ftdi220_drv.o ftdi220_hw.o ftdi220_prm.o $(ftdi220_vg-obj-yy) $(ftdi220-objs-yy)
else
ftdi220-objs := ftdi220_drv.o ftdi220_hw.o ftdi220_prm.o $(ftdi220-objs-yy)
endif

modules-yy += ftdi220.o

obj-m += $(modules-yy)
obj-m := $(sort $(obj-m))

else

modules:
	$(MAKE) -C $(LINUX_SRC) SDK_MODULE_SRC=$(SDK_MODULE_SRC) M=$(PWD) modules	
	$(STRIP) $(STRIPFLAGS) $(MODULE).ko

.PHONEY: clean
clean:
	$(MAKE) -C $(LINUX_SRC) M=$(PWD) clean

.PHONEY: all
all: modules

copy:
	cp -f $(MODULE).ko $(SDK_SRC)/target/rootfs-cpio/lib/modules/

endif
