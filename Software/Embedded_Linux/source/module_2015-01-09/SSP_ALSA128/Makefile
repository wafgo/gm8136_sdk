sinclude $(PWD)/../module_conf

PWD := $(shell pwd)
STRIPFLAGS = -g --strip-unneeded
STRIP = $(CROSS_COMPILE)strip

EXTRA_CFLAGS += -DFTSSP_DEBUG_MESSAGE

MODULE = snd_ftssp010

ifneq ($(KERNELRELEASE),)
$(MODULE)-objs-yy := ftssp010-pcm.o ftssp010-ssp.o debug.o
$(MODULE)-objs-y$(CONFIG_PLATFORM_GM8210) += ftssp010-gm8210.o
$(MODULE)-objs-y$(CONFIG_PLATFORM_GM8287) += ftssp010-gm8287.o

$(MODULE)-objs := $($(MODULE)-objs-yy)

obj-m += $(MODULE).o
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
