sinclude $(PWD)/../module_conf
PWD := $(shell pwd)
LINUX_SRC=$(SDK_SRC)linux-3.3-fa

EXTRA_CFLAGS += -I$(SDK_MODULE_SRC)/include/
STRIPFLAGS = -g --strip-unneeded
MODULE = dma_util
STRIP = $(CROSS_COMPILE)strip

ifneq ($(KERNELRELEASE),)
obj-m += $(MODULE).o
obj-m := $(sort $(obj-m))
else
ifndef SDK_MODULE_SRC
SDK_MODULE_SRC = $(SDK_SRC)/module/
endif

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
