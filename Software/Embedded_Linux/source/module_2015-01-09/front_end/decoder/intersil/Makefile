PWD := $(shell pwd)
sinclude $(PWD)/../../../module_conf

STRIPFLAGS = -g --strip-unneeded
STRIP = $(CROSS_COMPILE)strip

SDK_MODULE_SRC := $(SDK_SRC)module
KERNELDIR := $(LINUX_SRC)
FE_DEC_INCDIR := $(SDK_MODULE_SRC)/include/front_end/decoder

ifneq ($(KERNELRELEASE),)

include $(srctree)/.config

EXTRA_CFLAGS += -Wall -g -I$(obj)/../../ -I$(FE_DEC_INCDIR)/

tw9900-objs := tw9900_drv.o tw9900_lib.o
obj-m += tw9900.o

tw2865-objs := tw2865_drv.o tw2865_lib.o
obj-m += tw2865.o

tw2868-objs := tw2868_drv.o tw2868_lib.o
obj-m += tw2868.o

tw2968-objs := tw2968_drv.o tw2968_lib.o
obj-m += tw2968.o

tw2964-objs := tw2964_drv.o tw2964_lib.o
obj-m += tw2964.o

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
