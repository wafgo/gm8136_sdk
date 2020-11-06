PWD := $(shell pwd)
sinclude $(PWD)/../../module_conf

STRIPFLAGS = -g --strip-unneeded
STRIP = $(CROSS_COMPILE)strip

SDK_MODULE_SRC := $(SDK_SRC)module
KERNELDIR := $(LINUX_SRC)
FE_DEC_INCDIR := $(shell if [ -d $(SDK_MODULE_SRC)/include/front_end/decoder ]; then echo $(SDK_MODULE_SRC)/include/front_end/decoder; else echo ""; fi;)
FE_SDI_INCDIR := $(shell if [ -d $(SDK_MODULE_SRC)/include/front_end/sdi ]; then echo $(SDK_MODULE_SRC)/include/front_end/sdi; else echo ""; fi;)
FE_CVI_INCDIR := $(shell if [ -d $(SDK_MODULE_SRC)/include/front_end/hdcvi ]; then echo $(SDK_MODULE_SRC)/include/front_end/hdcvi; else echo ""; fi;)
FE_TVI_INCDIR := $(shell if [ -d $(SDK_MODULE_SRC)/include/front_end/hdtvi ]; then echo $(SDK_MODULE_SRC)/include/front_end/hdtvi; else echo ""; fi;)
FE_SEN_INCDIR := $(shell if [ -d $(SDK_MODULE_SRC)/include/front_end/sensor ]; then echo $(SDK_MODULE_SRC)/include/front_end/sensor; else echo ""; fi;)
FE_AHD_INCDIR := $(shell if [ -d $(SDK_MODULE_SRC)/include/front_end/ahd ]; then echo $(SDK_MODULE_SRC)/include/front_end/ahd; else echo ""; fi;)
ISP_INCDIR    := $(shell if [ -d $(SDK_MODULE_SRC)/include/isp320 ]; then echo $(SDK_MODULE_SRC)/include/isp320; else echo ""; fi;)

ifneq ($(KERNELRELEASE),)

EXTRA_CFLAGS += -Wall -g -I$(obj)/../

ifneq ($(FE_DEC_INCDIR),)
EXTRA_CFLAGS += -I$(FE_DEC_INCDIR)
endif

ifneq ($(FE_SDI_INCDIR),)
EXTRA_CFLAGS += -I$(FE_SDI_INCDIR)
endif

ifneq ($(FE_CVI_INCDIR),)
EXTRA_CFLAGS += -I$(FE_CVI_INCDIR)
endif

ifneq ($(FE_TVI_INCDIR),)
EXTRA_CFLAGS += -I$(FE_TVI_INCDIR)
endif

ifneq ($(FE_SEN_INCDIR),)
EXTRA_CFLAGS += -I$(FE_SEN_INCDIR)
endif

ifneq ($(FE_AHD_INCDIR),)
EXTRA_CFLAGS += -I$(FE_AHD_INCDIR)
endif

ifneq ($(ISP_INCDIR),)
EXTRA_CFLAGS += -I$(ISP_INCDIR)
endif

vcap300_generic_m0-objs := generic_m0.o
modules-y+= vcap300_generic_m0.o

vcap300_generic_m1-objs := generic_m1.o
modules-y+= vcap300_generic_m1.o

vcap300_generic_m2-objs := generic_m2.o
modules-y+= vcap300_generic_m2.o

vcap300_generic_m3-objs := generic_m3.o
modules-y+= vcap300_generic_m3.o

vcap300_generic_m4-objs := generic_m4.o
modules-y+= vcap300_generic_m4.o

vcap300_generic_m5-objs := generic_m5.o
modules-y+= vcap300_generic_m5.o

vcap300_generic_m6-objs := generic_m6.o
modules-y+= vcap300_generic_m6.o

vcap300_generic_m7-objs := generic_m7.o
modules-y+= vcap300_generic_m7.o

vcap300_generic_m8-objs := generic_m8.o
modules-y+= vcap300_generic_m8.o

ifneq ($(FE_DEC_INCDIR),)
vcap300_tw2865-objs := tw2865.o
modules-y+= vcap300_tw2865.o

vcap300_tw2868-objs := tw2868.o
modules-y+= vcap300_tw2868.o

vcap300_tw2964-objs := tw2964.o
modules-y+= vcap300_tw2964.o

vcap300_tw2968-objs := tw2968.o
modules-y+= vcap300_tw2968.o

vcap300_tw9900-objs := tw9900.o
modules-y+= vcap300_tw9900.o

vcap300_nvp1104-objs := nvp1104.o
modules-y+= vcap300_nvp1104.o

vcap300_nvp1118-objs := nvp1118.o
modules-y+= vcap300_nvp1118.o

vcap300_nvp1914-objs := nvp1914.o
modules-y+= vcap300_nvp1914.o

vcap300_nvp1914c-objs := nvp1914c.o
modules-y+= vcap300_nvp1914c.o

vcap300_nvp1918-objs := nvp1918.o
modules-y+= vcap300_nvp1918.o

vcap300_nvp1918c-objs := nvp1918c.o
modules-y+= vcap300_nvp1918c.o

vcap300_cx26848-objs := cx26848.o
modules-y+= vcap300_cx26848.o

vcap300_rn6314-objs := rn6314.o
modules-y+= vcap300_rn6314.o

vcap300_rn6318-objs := rn6318.o
modules-y+= vcap300_rn6318.o
endif

ifneq ($(FE_SEN_INCDIR),)
vcap300_hm1375-objs := hm1375.o
modules-y+= vcap300_hm1375.o

vcap300_mt9m131-objs := mt9m131.o
modules-y+= vcap300_mt9m131.o

vcap300_mt9d131-objs := mt9d131.o
modules-y+= vcap300_mt9d131.o

vcap300_ov7725-objs := ov7725.o
modules-y+= vcap300_ov7725.o
endif

ifneq ($(FE_SDI_INCDIR),)
vcap300_cx25930-objs := cx25930.o
modules-y+= vcap300_cx25930.o

vcap300_mv4101-objs := mv4101.o
modules-y+= vcap300_mv4101.o

vcap300_en331-objs := en331.o
modules-y+= vcap300_en331.o
endif

ifneq ($(FE_CVI_INCDIR),)
vcap300_dh9901-objs := dh9901.o
modules-y+= vcap300_dh9901.o
endif

ifneq ($(FE_TVI_INCDIR),)
vcap300_tp2802-objs := tp2802.o
modules-y+= vcap300_tp2802.o
endif

ifneq ($(FE_AHD_INCDIR),)
vcap300_nvp6114-objs := nvp6114.o
modules-y+= vcap300_nvp6114.o
vcap300_nvp6124-objs := nvp6124.o
modules-y+= vcap300_nvp6124.o
endif

ifneq ($(ISP_INCDIR),)
vcap300_isp-objs := isp.o
modules-y+= vcap300_isp.o
endif

obj-m += $(modules-y)
obj-m := $(sort $(obj-m))

else

all modules:
	$(MAKE) -C $(KERNELDIR) SDK_MODULE_SRC=$(SDK_MODULE_SRC) M=$(PWD) modules
	@$(STRIP) $(STRIPFLAGS) *.ko

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

endif
