PWD := $(shell pwd)
sinclude $(PWD)/../../../module_conf

STRIPFLAGS = -g --strip-unneeded
STRIP = $(CROSS_COMPILE)strip

SDK_MODULE_SRC := $(SDK_SRC)module
KERNELDIR := $(LINUX_SRC)
FE_SENSOR_INCDIR := $(SDK_MODULE_SRC)/include/front_end/sensor

ifneq ($(KERNELRELEASE),)

include $(srctree)/.config

EXTRA_CFLAGS += -Wall -g -I$(obj)/../../ -I$(FE_SENSOR_INCDIR)/

hm1375-objs := hm1375_drv.o
obj-m += hm1375.o

obj-m += $(modules-y)
obj-m := $(sort $(obj-m))

else

all modules:
	$(MAKE) -C $(KERNELDIR) SDK_MODULE_SRC=$(SDK_MODULE_SRC) M=$(PWD) modules
	@$(STRIP) $(STRIPFLAGS) *.ko

module_strip:
	@$(STRIP) $(STRIPFLAGS) *.ko

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

endif