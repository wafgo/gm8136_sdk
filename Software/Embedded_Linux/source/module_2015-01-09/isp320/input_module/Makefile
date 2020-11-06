ifeq ($(KERNELRELEASE),)
PWD := $(shell pwd)
sinclude $(PWD)/../../module_conf

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

common-objs := spi_common.o

fisp_iris_da-objs := $(common-objs) iris_da.o
modules-y+= fisp_iris_da.o

fisp_iris_pwm-objs := $(common-objs) iris_pwm.o
modules-y+= fisp_iris_pwm.o

fisp_ar0331-objs := $(common-objs) sen_ar0331.o
modules-y+= fisp_ar0331.o

fisp_mn34031-objs := $(common-objs) sen_mn34031.o
modules-y+= fisp_mn34031.o

fisp_imx076-objs := $(common-objs) sen_imx076.o
modules-y+= fisp_imx076.o

fisp_imx036-objs := $(common-objs) sen_imx036.o
modules-y+= fisp_imx036.o

fisp_imx122-objs := $(common-objs) sen_imx122.o
modules-y+= fisp_imx122.o

fisp_imx124-objs := $(common-objs) sen_imx124.o
modules-y+= fisp_imx124.o


fisp_imx138-objs := $(common-objs) sen_imx138.o
modules-y+= fisp_imx138.o

fisp_soih22-objs := $(common-objs) sen_soih22.o
modules-y+= fisp_soih22.o

fisp_mt9p031-objs := $(common-objs) sen_mt9p031.o
modules-y+= fisp_mt9p031.o

fisp_ov5653-objs := $(common-objs) sen_ov5653.o
modules-y+= fisp_ov5653.o

fisp_ov5658-objs := $(common-objs) sen_ov5658.o
modules-y+= fisp_ov5658.o

fisp_ov4689-objs := $(common-objs) sen_ov4689.o
modules-y+= fisp_ov4689.o

fisp_GC1004-objs := $(common-objs) sen_GC1004.o
modules-y+= fisp_GC1004.o

fisp_ov2715-objs := $(common-objs) sen_ov2715.o
modules-y+= fisp_ov2715.o

fisp_ov9715-objs := $(common-objs) sen_ov9715.o
modules-y+= fisp_ov9715.o

fisp_mt9m034-objs := $(common-objs) sen_mt9m034.o
modules-y+= fisp_mt9m034.o

fisp_mt9p031-objs := $(common-objs) sen_mt9p031.o
modules-y+= fisp_mt9p031.o

fisp_ar0140-objs := $(common-objs) sen_ar0140.o
modules-y+= fisp_ar0140.o
fisp_ar0835-objs := $(common-objs) sen_ar0835.o
modules-y+= fisp_ar0835.o
fisp_imx136-objs := $(common-objs) sen_imx136.o
modules-y+= fisp_imx136.o
fisp_imx104-objs := $(common-objs) sen_imx104.o
modules-y+= fisp_imx104.o
fisp_mn34041-objs := $(common-objs) sen_mn34041.o
modules-y+= fisp_mn34041.o
fisp_imx169-objs := $(common-objs) sen_imx169.o
modules-y+= fisp_imx169.o
fisp_imx179-objs := $(common-objs) sen_imx179.o
modules-y+= fisp_imx179.o
fisp_ov9714-objs := $(common-objs) sen_ov9714.o
modules-y+= fisp_ov9714.o
fisp_ar0330-objs := $(common-objs) sen_ar0330.o
modules-y+= fisp_ar0330.o
fisp_mn34220-objs := $(common-objs) sen_mn34220.o
modules-y+= fisp_mn34220.o
fisp_ps1210-objs := $(common-objs) sen_ps1210.o
modules-y+= fisp_ps1210.o
fisp_lens_9510a2-objs := $(common-objs) lens_9510a2.o
modules-y+= fisp_lens_9510a2.o
fisp_lens_td9533t-objs := $(common-objs) lens_td9533t.o
modules-y+= fisp_lens_td9533t.o

ifneq ($(KERNELRELEASE),)
EXTRA_CFLAGS += -I$(obj)/../
EXTRA_CFLAGS += -I$(SDK_MODULE_SRC)/include/isp320
EXTRA_CFLAGS += -I$(SDK_MODULE_SRC)/include/pwm
obj-m += $(modules-y)
obj-m := $(sort $(obj-m))
else
include $(PWD)/../config.mak
modules:
	$(MAKE) -C $(LINUX_SRC) SDK_MODULE_SRC=$(SDK_MODULE_SRC) M=$(PWD) modules
	@$(STRIP) $(STRIPFLAGS) *.ko
	@cp *.ko ../ko/

clean:
	$(MAKE) -C $(LINUX_SRC) M=$(PWD) clean
endif
