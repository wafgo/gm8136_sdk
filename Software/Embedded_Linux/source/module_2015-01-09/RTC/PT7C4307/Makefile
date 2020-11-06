PWD := $(shell pwd)
sinclude $(PWD)/../../module_conf

STRIPFLAGS = -g --strip-unneeded
STRIP = $(CROSS_COMPILE)strip

SDK_MODULE_SRC := $(SDK_SRC)module
SDK_MODULE_INCDIR := $(SDK_MODULE_SRC)/include
KERNELDIR := $(LINUX_SRC)

MODULE = rtc-pt7c4307

ifneq ($(KERNELRELEASE),)

obj-m += $(MODULE).o

else

all modules:
	$(MAKE) -C $(KERNELDIR) SDK_MODULE_SRC=$(SDK_MODULE_SRC) M=$(PWD) modules
	@$(STRIP) $(STRIPFLAGS) $(MODULE).ko

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

endif
