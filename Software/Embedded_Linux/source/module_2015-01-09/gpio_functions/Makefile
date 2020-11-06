sinclude $(PWD)/../module_conf
PWD := $(shell pwd)

EXTRA_CFLAGS += -fno-common -DMPFX=$(MODULE) -DEXPORT_SYMTAB -I$(SDK_MODULE_SRC)/include/gpio_functions
STRIPFLAGS = -g --strip-unneeded
MODULE = ftgpio_functions
STRIP = $(CROSS_COMPILE)strip

ifneq ($(KERNELRELEASE),)
#$(MODULE)-objs := gpio_special_use.o
obj-m += $(MODULE).o
obj-m := $(sort $(obj-m))
else
ifndef SDK_MODULE_SRC
SDK_MODULE_SRC = $(SDK_SRC)/module/
endif

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
