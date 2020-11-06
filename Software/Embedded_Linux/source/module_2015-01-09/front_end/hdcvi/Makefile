PWD := $(shell pwd)
sinclude $(PWD)/../../module_conf

STRIPFLAGS = -g --strip-unneeded
STRIP = $(CROSS_COMPILE)strip

SDK_MODULE_SRC := $(SDK_SRC)module
KERNELDIR := $(LINUX_SRC)

ifneq ($(KERNELRELEASE),)

obj-m += dahua/
obj-m := $(sort $(obj-m))

else

all modules:
	$(MAKE) -C $(KERNELDIR) SDK_MODULE_SRC=$(SDK_MODULE_SRC) M=$(PWD) modules
	@$(MAKE) -C dahua/  module_lib
	@$(MAKE) -C dahua/  module_strip

module_lib:
	@$(MAKE) -C dahua/  module_lib

module_strip:
	@$(MAKE) -C dahua/  module_strip

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

endif