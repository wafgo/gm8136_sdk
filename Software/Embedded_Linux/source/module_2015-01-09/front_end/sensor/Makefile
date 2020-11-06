PWD := $(shell pwd)
sinclude $(PWD)/../../module_conf

STRIPFLAGS = -g --strip-unneeded
STRIP = $(CROSS_COMPILE)strip

SDK_MODULE_SRC := $(SDK_SRC)module
KERNELDIR := $(LINUX_SRC)

ifneq ($(KERNELRELEASE),)

obj-m += aptina/
obj-m := $(sort $(obj-m))

obj-m += himax/
obj-m := $(sort $(obj-m))

obj-m += omni_vision/
obj-m := $(sort $(obj-m))

else

all modules:
	$(MAKE) -C $(KERNELDIR) SDK_MODULE_SRC=$(SDK_MODULE_SRC) M=$(PWD) modules
	@$(MAKE) -C aptina/       module_strip
	@$(MAKE) -C himax/        module_strip
	@$(MAKE) -C omni_vision/  module_strip

module_strip:
	@$(MAKE) -C aptina/       module_strip
	@$(MAKE) -C himax/        module_strip
	@$(MAKE) -C omni_vision/  module_strip

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

endif