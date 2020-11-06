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

nvp1104-objs := nvp1104_drv.o nvp1104_lib.o
obj-m += nvp1104.o

nvp1918-objs := nvp1918_lib.o nvp1918_drv.o
obj-m += nvp1918.o

nvp1918c-objs := nvp1918c_lib.o nvp1918c_drv.o
obj-m += nvp1918c.o

nvp1118-objs := nvp1118_drv.o nvp1118_lib.o
obj-m += nvp1118.o

nvp1914-objs := nvp1914_drv.o nvp1914_lib.o
obj-m += nvp1914.o

nvp1914c-objs := nvp1914c_drv.o nvp1914c_lib.o
obj-m += nvp1914c.o

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
