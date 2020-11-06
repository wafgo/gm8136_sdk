MODULE = mdin340
PWD := $(shell pwd)
sinclude $(PWD)/../../module_conf
SDK_MODULE_SRC := $(SDK_SRC)module/

EXTRA_CFLAGS+=-I$(SDK_MODULE_SRC)/LCD200_v3 -I$(SDK_MODULE_SRC)/include/lcd200_v3
EXTRA_CFLAGS+=-D__MDINPCI_H__ -D__MDINOSD_H__ -D__MDINCUR_H__ -D__MDINGAC_H__ -D__MDINBUS_H__
STRIPFLAGS = -g --strip-unneeded
STRIP = $(CROSS_COMPILE)strip

ifneq ($(KERNELRELEASE),)
mdin340-objs := main.o app340.o mdin3xx.o mdinaux.o mdincoef.o mdinfrmt.o mdinhtx.o mdinipc.o

modules-yy += $(MODULE).o

obj-m += $(modules-yy)
obj-m := $(sort $(obj-m))

else


modules:
	$(MAKE) -C $(LINUX_SRC) SDK_MODULE_SRC=$(SDK_MODULE_SRC) M=$(PWD) modules

.PHONEY: clean
clean:
	$(MAKE) -C $(LINUX_SRC) M=$(PWD) clean

.PHONEY: all
all: modules

copy:
	cp -f $(MODULE).ko $(SDK_SRC)/target/rootfs-cpio/lib/modules/
	
endif
