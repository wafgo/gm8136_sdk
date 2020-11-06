MODULE = irda
sinclude $(PWD)/../module_conf

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







EXTRA_CFLAGS += -g -I$(SDK_MODULE_INC)irda
EXTRA_CFLAGS += -fno-common

STRIPFLAGS = -g --strip-unneeded
STRIP = $(CROSS_COMPILE)strip

RAM_TARGET = $(SDK_SRC)target/rootfs-cpio/lib/modules

ifneq ($(KERNELRELEASE),)
$(MODULE)-objs := platform.o irda_drv.o irda_dev.o 

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





