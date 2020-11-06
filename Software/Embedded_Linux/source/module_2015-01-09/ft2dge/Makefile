MODULE = ft2dge
sinclude $(PWD)/../module_conf

SDK_MODULE_SRC := $(SDK_SRC)module/
STRIPFLAGS = -g --strip-unneeded
STRIP = $(CROSS_COMPILE)strip

ifeq ($(CONFIG_PLATFORM_GM8210),y)
USING_FT2D_OSG=y
else
USING_FT2D_OSG=n
endif


ifneq ($(KERNELRELEASE),)

EXTRA_CFLAGS += -g -I$(SDK_MODULE_SRC)/include

ifeq ($(USING_FT2D_OSG),y)
EXTRA_CFLAGS += -DFT2D_OSG_USING
endif


ft2dge-drv-objs := ft2dge_drv.o

ft2dge-objs-y$(CONFIG_PLATFORM_GM8210) += gm8210.o

$(MODULE)-objs := $(ft2dge-drv-objs) $(ft2dge-objs-yy)

obj-m += $(MODULE).o

obj-m := $(sort $(obj-m))

else

PWD := $(shell pwd)

modules:
	$(MAKE) -C $(LINUX_SRC) M=$(PWD) modules
	$(STRIP) $(STRIPFLAGS) $(MODULE).ko
	cp *.ko /tmp

.PHONEY: clean
clean:
	$(MAKE) -C $(LINUX_SRC) M=$(PWD) clean

copy:
	cp -f $(MODULE).ko $(SDK_SRC)/target/rootfs-cpio/lib/modules/

endif
