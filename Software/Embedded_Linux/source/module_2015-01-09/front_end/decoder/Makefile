PWD := $(shell pwd)
sinclude $(PWD)/../../module_conf

STRIPFLAGS = -g --strip-unneeded
STRIP = $(CROSS_COMPILE)strip

SDK_MODULE_SRC := $(SDK_SRC)module
KERNELDIR := $(LINUX_SRC)

ifneq ($(KERNELRELEASE),)

obj-m += nextchip/
obj-m += intersil/
obj-m += conexant/
obj-m += richnex/
obj-m += everest/
obj-m := $(sort $(obj-m))

else

all modules:
	$(MAKE) -C $(KERNELDIR) SDK_MODULE_SRC=$(SDK_MODULE_SRC) M=$(PWD) modules
	@$(MAKE) -C nextchip/ module_strip
	@$(MAKE) -C intersil/ module_strip
	@$(MAKE) -C conexant/ module_strip
	@$(MAKE) -C richnex/  module_strip
	@$(MAKE) -C everest/  module_strip

module_strip:
	@$(MAKE) -C nextchip/ module_strip
	@$(MAKE) -C intersil/ module_strip
	@$(MAKE) -C conexant/ module_strip
	@$(MAKE) -C richnex/  module_strip
	@$(MAKE) -C everest/  module_strip

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

endif
