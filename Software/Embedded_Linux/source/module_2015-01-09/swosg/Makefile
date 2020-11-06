MODULE = sw_osg
sinclude $(PWD)/../module_conf

ifeq ($(8210_hw),y)
8210_OSG_USE_HW=y
endif
SWOSG_USING_API=y

ifeq ($(obj),)
obj=$(PWD)
endif

ifndef SDK_MODULE_SRC
VERS2614 = $(findstring arm-linux-2.6,$(obj))
VERS2628 = $(findstring arm-linux-2.6.28,$(obj))
VERS3300 = $(findstring arm-linux-3.3,$(obj))

ifneq ($(VERS3300),)
#SDK_SRC := /usr/src/arm-linux-3.3/
LINUX_SRC = $(SDK_SRC)linux-3.3-fa/
SDK_MODULE_SRC = $(SDK_SRC)module/
SDK_MODULE_INC = $(SDK_MODULE_SRC)include/

else ifneq ($(VERS2628),)
#SDK_SRC := /usr/src/arm-linux-2.6.28/
LINUX_SRC = $(SDK_SRC)linux-2.6.28-fa/
SDK_MODULE_SRC = $(SDK_SRC)module/
SDK_MODULE_INC = $(SDK_MODULE_SRC)include/

else
#SDK_SRC := /usr/src/arm-linux-2.6/
LINUX_SRC = $(SDK_SRC)linux-2.6.14-fa/
SDK_MODULE_SRC = $(SDK_SRC)module/
SDK_MODULE_INC = $(SDK_MODULE_SRC)
endif

endif


ifeq ($(SWOSG_USING_API),y)
EXTRA_CFLAGS+= -I$(SDK_MODULE_INC)
EXTRA_CFLAGS+= -fno-common
else
EXTRA_CFLAGS += -g  -I$(SDK_MODULE_SRC)/include/videograph.include/log -I$(SDK_MODULE_SRC)/include/videograph.include/em 
EXTRA_CFLAGS += -fno-common
endif

STRIPFLAGS = -g --strip-unneeded
STRIP = $(CROSS_COMPILE)strip

RAM_TARGET = $(SDK_SRC)target/rootfs-cpio/lib/modules

ifneq ($(KERNELRELEASE),)
ifeq ($(SWOSG_USING_API),y)

swosg-objs-y$(CONFIG_PLATFORM_GM8287) += swosg_8287.o
ifeq ($(8210_OSG_USE_HW),y)
swosg-objs-y$(CONFIG_PLATFORM_GM8210) += swosg_8210_hw.o
else
swosg-objs-y$(CONFIG_PLATFORM_GM8210) += swosg_8210.o
endif
swosg-objs-y$(CONFIG_PLATFORM_GM8139) += swosg_8139.o

swosg-objs-y$(CONFIG_PLATFORM_GM8136) += swosg_8136.o
#swosg-objs-yy += swosg_8210.o
$(MODULE)-objs := swosg_if.o $(swosg-objs-yy)

else
$(MODULE)-objs := swosg_vg.o swosg_drv.o
endif
obj-m += $(MODULE).o
obj-m := $(sort $(obj-m))

else
PWD := $(shell pwd)


modules:

	$(MAKE) -C $(LINUX_SRC) M=$(PWD) modules
	$(STRIP) $(STRIPFLAGS) $(MODULE).ko

ifdef RAM_TARGET
	cp *.ko $(RAM_TARGET)
endif	
clean:
	$(MAKE) -C $(LINUX_SRC) M=$(PWD) clean
endif
