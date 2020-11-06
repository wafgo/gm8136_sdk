MODULE = osd_dispatch

sinclude $(PWD)/../module_conf
STRIPFLAGS = -g --strip-unneeded
STRIP = $(CROSS_COMPILE)strip

SDK_MODULE_SRC := $(SDK_SRC)module/
SDK_MODULE_INCDIR := $(SDK_MODULE_SRC)/include
KERNELDIR := $(LINUX_SRC)

ifneq ($(KERNELRELEASE),)

EXTRA_CFLAGS += -g -I$(SDK_MODULE_INCDIR) -I$(SDK_MODULE_INCDIR)/osd_dispatch -I$(SDK_MODULE_INCDIR)/scaler300 \
                -I$(SDK_MODULE_INCDIR)/vcap300

obj-m += $(MODULE).o
obj-m := $(sort $(obj-m))

else

PWD := $(shell pwd)

modules:

	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
	$(STRIP) $(STRIPFLAGS) $(MODULE).ko

.PHONEY: clean
clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

.PHONEY: all
all: modules

copy:
	cp -f $(MODULE).ko $(SDK_SRC)/target/rootfs-cpio/lib/modules/

endif
