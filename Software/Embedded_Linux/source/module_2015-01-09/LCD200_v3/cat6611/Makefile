MODULE = cat6611

sinclude $(PWD)/../../module_conf

SDK_MODULE_SRC := $(SDK_SRC)module/

EXTRA_CFLAGS+=-I$(SDK_MODULE_SRC)/LCD200_v2 -I$(SDK_MODULE_SRC)/CT656 -I$(SDK_MODULE_SRC)/include/lcd200_v3

STRIPFLAGS = -g --strip-unneeded
STRIP = $(CROSS_COMPILE)strip

ifneq ($(KERNELRELEASE),)
cat6611-objs := cat6611_drv.o cat6611_sys.o mcuio.o main.o

modules-yy += $(MODULE).o

obj-m += $(modules-yy)
obj-m := $(sort $(obj-m))

else

PWD := $(shell pwd)

modules:
	$(MAKE) -C $(LINUX_SRC) M=$(PWD) modules
	$(STRIP) $(STRIPFLAGS) $(MODULE).ko

.PHONEY: clean
clean:
	$(MAKE) -C $(LINUX_SRC) M=$(PWD) clean

.PHONEY: all
all: modules

copy:
	cp -f $(MODULE).ko $(SDK_SRC)/target/rootfs-cpio/lib/modules/

endif
