MODULE = flcd200-common
MODULE0 = flcd200-pip
MODULE1 = flcd200-pip1
MODULE2 = flcd200-pip2

sinclude $(PWD)/../module_conf

SDK_MODULE_SRC := $(SDK_SRC)module/

include $(LINUX_SRC)/.config

STRIPFLAGS = -g --strip-unneeded
STRIP = $(CROSS_COMPILE)strip

ifneq ($(KERNELRELEASE),)

EXTRA_CFLAGS+=-I$(SDK_MODULE_SRC)/include/videograph  -I$(SDK_MODULE_SRC)/include/frammap -I$(SDK_MODULE_SRC)/include/lcd200_v3 -I$(SDK_MODULE_SRC)/include/
EXTRA_CFLAGS+=-I$(SDK_MODULE_SRC)/videograph
EXTRA_CFLAGS+=-I$(SDK_MODULE_SRC)/include/videograph.include/log -I$(SDK_MODULE_SRC)/include/videograph.include/em

common-objs := ffb.o proc.o debug.o codec.o frame_comm.o cursor_comm.o
$(MODULE)-objs-yy := $(common-objs)
$(MODULE)-objs-y$(CONFIG_PLATFORM_GM8126) += gm8126.o
$(MODULE)-objs-y$(CONFIG_PLATFORM_GM8210) += gm8210.o
$(MODULE)-objs-y$(CONFIG_PLATFORM_GM8287) += gm8287.o
$(MODULE)-objs-y$(CONFIG_PLATFORM_GM8139) += gm8139.o
$(MODULE)-objs-y$(CONFIG_PLATFORM_GM8136) += gm8136.o

$(MODULE)-objs := $($(MODULE)-objs-yy)

modules-yy := $(MODULE).o

CFLAGS_pip.o = $(PIP_DEFINES)
$(MODULE0)-objs := pip.o dev.o simple_osd.o
$(MODULE1)-objs := pip.o dev.o simple_osd.o
$(MODULE2)-objs := pip.o dev.o simple_osd.o

ifeq ($(CONFIG_PLATFORM_GM8126),y)
CONFIG_FILE=config_8126
endif

ifeq ($(CONFIG_PLATFORM_GM8210),y)
CONFIG_FILE=config_8210
CONFIG1_FILE=config1_8210
CONFIG2_FILE=config2_8210
endif

ifeq ($(CONFIG_PLATFORM_GM8139),y)
CONFIG_FILE=config_8139
endif

ifeq ($(CONFIG_PLATFORM_GM8136),y)
CONFIG_FILE=config_8136
endif


ifeq ($(CONFIG_PLATFORM_GM8287),y)
CONFIG_FILE=config_8287
CONFIG2_FILE=config2_8287
endif

EXTRA_CFLAGS+=-DVIDEOGRAPH_INC=1

ifdef LCD_DEV2
include $(obj)/$(CONFIG2_FILE)
include $(obj)/Makefile.rules
MODULE_SUB = $(MODULE2)
EXTRA_CFLAGS+=-DLCD_DEV2
else
ifdef LCD_DEV1
include $(obj)/$(CONFIG1_FILE)
include $(obj)/Makefile.rules
MODULE_SUB = $(MODULE1)
EXTRA_CFLAGS+=-DLCD_DEV1
else
include $(obj)/$(CONFIG_FILE)
include $(obj)/Makefile.rules
MODULE_SUB = $(MODULE0)
modules-yy += $(MODULE).o
endif
endif

modules-yy += $(MODULE_SUB).o

obj-m += $(modules-yy)
obj-m := $(sort $(obj-m))

else

ifndef SDK_MODULE_SRC
SDK_MODULE_SRC = $(SDK_SRC)/module/
endif

modules:
	$(MAKE) -C $(LINUX_SRC) SDK_MODULE_SRC=$(SDK_MODULE_SRC) M=$(PWD) modules 

ifeq ($(CONFIG_PLATFORM_GM8210),y)
	$(MAKE) -C $(LINUX_SRC) SDK_MODULE_SRC=$(SDK_MODULE_SRC) M=$(PWD) LCD_DEV1=1 modules 
	$(MAKE) -C $(LINUX_SRC) SDK_MODULE_SRC=$(SDK_MODULE_SRC) M=$(PWD) LCD_DEV2=1 modules
endif

ifeq ($(CONFIG_PLATFORM_GM8287),y)
	$(MAKE) -C $(LINUX_SRC) SDK_MODULE_SRC=$(SDK_MODULE_SRC) M=$(PWD) LCD_DEV2=1 modules
endif
	find -name \*.ko | xargs $(STRIP) $(STRIPFLAGS)

	
.PHONEY: clean
clean:
	$(MAKE) -C $(LINUX_SRC) M=$(PWD) clean
    
.PHONEY: all
all: modules

.PHONEY: copy
copy:
	find -name \*.ko -exec cp {} $(SDK_SRC)target/rootfs-cpio/lib/modules/ \;
        
endif
