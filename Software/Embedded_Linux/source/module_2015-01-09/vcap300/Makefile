PWD := $(shell pwd)
sinclude $(PWD)/../module_conf

STRIPFLAGS = -g --strip-unneeded
STRIP = $(CROSS_COMPILE)strip

SDK_MODULE_SRC := $(SDK_SRC)module
SDK_MODULE_INCDIR := $(SDK_MODULE_SRC)/include
KERNELDIR := $(LINUX_SRC)

VCAP_API_INCDIR := $(SDK_MODULE_INCDIR)/vcap300
FRAMMAP_INCDIR := $(SDK_MODULE_INCDIR)/frammap
VG_INCDIR := $(SDK_MODULE_INCDIR)/videograph.include

MODULE_INCDIR := -I$(SDK_MODULE_INCDIR) -I$(VCAP_API_INCDIR) -I$(FRAMMAP_INCDIR) \
                 -I$(VG_INCDIR) -I$(VG_INCDIR)/em -I$(VG_INCDIR)/log

ifneq ($(KERNELRELEASE),)

include $(srctree)/.config

ifeq ($(CONFIG_PLATFORM_A369),y)
CONFIG_FILE=config_a369
PLATFORM_OBJ=plat_a369.o
endif

ifeq ($(CONFIG_PLATFORM_GM8210),y)
CONFIG_FILE=config_8210
PLATFORM_OBJ=plat_8210.o
endif

ifeq ($(CONFIG_PLATFORM_GM8287),y)
CONFIG_FILE=config_8287
PLATFORM_OBJ=plat_8287.o
endif

ifeq ($(CONFIG_PLATFORM_GM8139),y)
CONFIG_FILE=config_8139
PLATFORM_OBJ=plat_8139.o
endif

ifeq ($(CONFIG_PLATFORM_GM8136),y)
CONFIG_FILE=config_8136
PLATFORM_OBJ=plat_8136.o
endif

include $(obj)/$(CONFIG_FILE)
include $(obj)/config
include $(obj)/Makefile.rules

EXTRA_CFLAGS += $(PLATFORM_CFLAGS)
EXTRA_CFLAGS += -Wall -g $(MODULE_INCDIR)

VG_OBJS = vcap_vg.o
COMM_OBJS = vcap_drv.o vcap_proc.o vcap_input.o vcap_i2c.o vcap_lli.o vcap_osd.o vcap_palette.o vcap_lib.o \
            vcap_fcs.o vcap_dn.o vcap_presmo.o vcap_sharp.o vcap_md.o vcap_mark.o vcap_mask.o vcap_api.o

MODULE := vcap300_common
$(MODULE)-objs = $(COMM_OBJS) $(VG_OBJS) $(PLATFORM_OBJ)

vcap0-objs := vcap_dev.o
modules-yy += vcap0.o

obj-m += $(modules-yy) $(MODULE).o
obj-m += input_module/
obj-m := $(sort $(obj-m))

else

all modules:
	$(MAKE) -C $(KERNELDIR) SDK_MODULE_SRC=$(SDK_MODULE_SRC) M=$(PWD) modules
	@$(STRIP) $(STRIPFLAGS) *.ko input_module/*.ko

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

dist:
	@mv vcap_input.h vcap_input.dist
	@mv vcap_dbg.h vcap_dbg.dist
	@rm -f *.c *.h config* Makefile*
	@mv vcap_input.dist vcap_input.h
	@mv vcap_dbg.dist vcap_dbg.h

endif
