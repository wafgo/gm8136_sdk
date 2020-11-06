PWD := $(shell pwd)
sinclude $(PWD)/../../module_conf

STRIPFLAGS = -g --strip-unneeded
STRIP = $(CROSS_COMPILE)strip

SDK_MODULE_SRC := $(SDK_SRC)module
SDK_MODULE_INCDIR := $(SDK_MODULE_SRC)/include
KERNELDIR := $(LINUX_SRC)

MODULE = rtc-ftrtc011

ifneq ($(KERNELRELEASE),)

include $(srctree)/.config

ifeq ($(CONFIG_PLATFORM_GM8139),y)
PLATFORM_OBJ=plat_8139.o
endif

ifeq ($(CONFIG_PLATFORM_GM8136),y)
PLATFORM_OBJ=plat_8136.o
endif

$(MODULE)-objs = $(PLATFORM_OBJ) ftrtc011.o

obj-m += $(MODULE).o
obj-m := $(sort $(obj-m))

else

all modules:
	$(MAKE) -C $(KERNELDIR) SDK_MODULE_SRC=$(SDK_MODULE_SRC) M=$(PWD) modules
	@$(STRIP) $(STRIPFLAGS) $(MODULE).ko

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

endif
