MODULE = sli10121

sinclude $(PWD)/../../module_conf
SDK_MODULE_SRC := $(SDK_SRC)module/
LINUX_INC=$(LINUX_SRC)/include

EXTRA_CFLAGS+=-I$(SDK_MODULE_SRC)/include/lcd200_v3 -I$(SDK_MODULE_SRC)/include/ -I$(SDK_MODULE_SRC)/LCD200_v3 -DKENNY_CHANGE


STRIPFLAGS = -g --strip-unneeded
STRIP = $(CROSS_COMPILE)strip

ifneq ($(KERNELRELEASE),)

gm_i2c-y$(CONFIG_PLATFORM_GM8210) := gm_apb.o
gm_i2c-y$(CONFIG_PLATFORM_GM8287) := gm_i2c.o

sli10121-objs := main.o HDMI_function.o $(gm_i2c-yy)

modules-yy += sli10121.o

obj-m += $(modules-yy)
obj-m := $(sort $(obj-m))

else

PWD := $(shell pwd)


modules:
	$(MAKE) -C $(LINUX_SRC) SDK_MODULE_SRC=$(SDK_MODULE_SRC) M=$(PWD) modules
	$(STRIP) $(STRIPFLAGS) $(MODULE).ko

.PHONEY: clean	
clean:
	$(MAKE) -C $(LINUX_SRC) M=$(PWD) clean

.PHONEY: copy 
copy:
	find -name \*.ko -exec cp {} $(SDK_SRC)target/rootfs-cpio/lib/modules/ \;

endif
