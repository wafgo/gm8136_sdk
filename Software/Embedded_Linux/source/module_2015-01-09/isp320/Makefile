ifeq ($(KERNELRELEASE),)
PWD := $(shell pwd)
sinclude $(PWD)/../module_conf

ifndef SDK_MODULE_SRC
SDK_MODULE_SRC = $(SDK_SRC)/module/
endif
endif

ifeq ($(obj),)
obj=$(PWD)
endif

ifeq ($(srctree),)
include $(SDK_SRC)linux-3.3-fa/.config
else
include $(srctree)/.config
endif

ifeq ($(CONFIG_PLATFORM_GM8139),y)
PLATFORM_OBJ=plat_8139.o
endif

MODULE := fisp320
$(MODULE)-objs := util_cfg.o isp_cfg.o isp_hal.o isp_job.o isp_dev.o isp_proc.o isp_drv.o isp_lib.o isp_api.o\
                  isp_input_inf.o isp_alg_inf.o $(PLATFORM_OBJ)

EXTRA_CFLAGS+=$(PLATFORM_CFLAGS)
EXTRA_CFLAGS+=-DMPFX=fisp
EXTRA_CFLAGS+=-I$(SDK_MODULE_SRC)/include/isp320
EXTRA_CFLAGS+=-I$(SDK_MODULE_SRC)/include/ft3dnr
EXTRA_CFLAGS+=-I$(SDK_MODULE_SRC)/include/frammap

ifneq ($(KERNELRELEASE),)
obj-m +=$(modules-yy)
obj-m +=$(MODULE).o
obj-m := $(sort $(obj-m))
else
include $(PWD)/config.mak
modules:
	$(MAKE) -C $(LINUX_SRC) SDK_MODULE_SRC=$(SDK_MODULE_SRC) M=$(PWD) modules
	@$(STRIP) $(STRIPFLAGS) *.ko
	@mkdir -p ko
	@cp *.ko $(PWD)/ko/
	@make -C $(PWD)/input_module
	@cp ko/*.ko ../../target/rootfs-cpio/lib/modules/

clean:
	$(MAKE) -C $(LINUX_SRC) M=$(PWD) clean
	@rm -f input_module/*.order
	@rm -f ./ko/*.ko

all:
	@make modules
	@make -C $(PWD)/isp_algorithm

cleanall:
	@make -C $(PWD) clean
	@make -C $(PWD)/isp_algorithm clean

endif
