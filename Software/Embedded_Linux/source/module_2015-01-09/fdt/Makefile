sinclude $(PWD)/../module_conf
PWD := $(shell pwd)

#VIDEOGRAPH_INC=1
MODULE =fdt

ifeq ($(obj),)
obj=$(PWD)
endif

SDK_MODULE_SRC := $(SDK_SRC)module/
STRIPFLAGS = -g --strip-unneeded
STRIP = $(CROSS_COMPILE)strip


#yes for 8210 IC, others for 8210 FPGA


EXTRA_CFLAGS += -I$(SDK_MODULE_SRC)/include/frammap -I$(SDK_MODULE_SRC)/include
EXTRA_CFLAGS += -DLINUX 

ifneq ($(KERNELRELEASE),)
# for ioctl driver
top-objs := fdt_alg.o fdt_ioctl.o

$(MODULE)-objs := $(top-objs)
modules-yy := $(MODULE).o

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
