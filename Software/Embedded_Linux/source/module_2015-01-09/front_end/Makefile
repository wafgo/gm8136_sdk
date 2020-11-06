PWD := $(shell pwd)
sinclude $(PWD)/../module_conf

STRIPFLAGS = -g --strip-unneeded
STRIP = $(CROSS_COMPILE)strip

SDK_MODULE_SRC := $(SDK_SRC)module
KERNELDIR := $(LINUX_SRC)

ifneq ($(KERNELRELEASE),)

include $(srctree)/.config

EXTRA_CFLAGS += -Wall -g

ifeq ($(CONFIG_PLATFORM_A369),y)
PLATFORM_OBJ=plat_a369.o
endif

ifeq ($(CONFIG_PLATFORM_GM8210),y)
PLATFORM_OBJ=plat_8210.o
endif

ifeq ($(CONFIG_PLATFORM_GM8287),y)
PLATFORM_OBJ=plat_8287.o
endif

ifeq ($(CONFIG_PLATFORM_GM8139),y)
PLATFORM_OBJ=plat_8139.o
endif

ifeq ($(CONFIG_PLATFORM_GM8136),y)
PLATFORM_OBJ=plat_8136.o
endif

MODULE := fe_common
$(MODULE)-objs = $(PLATFORM_OBJ)

obj-m += $(MODULE).o
obj-m += decoder/
obj-m += sdi/
obj-m += ahd/
obj-m += sensor/
obj-m += hdtvi/
obj-m += hdcvi/
obj-m += hybrid/
obj-m := $(sort $(obj-m))

else

all modules:
	$(MAKE) -C $(KERNELDIR) SDK_MODULE_SRC=$(SDK_MODULE_SRC) M=$(PWD) modules
	@$(MAKE) -C decoder/ module_strip
	@$(MAKE) -C sdi/     module_strip
	@$(MAKE) -C ahd/     module_strip
	@$(MAKE) -C sensor/  module_strip
	@$(MAKE) -C hdtvi/   module_strip
	@$(MAKE) -C hdcvi/   module_lib
	@$(MAKE) -C hdcvi/   module_strip
	@$(MAKE) -C hybrid/  module_strip
	@$(STRIP) $(STRIPFLAGS) *.ko

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

endif
