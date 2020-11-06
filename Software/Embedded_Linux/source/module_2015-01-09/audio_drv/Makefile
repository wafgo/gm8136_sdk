sinclude $(PWD)/../module_conf
PWD := $(shell pwd)

VIDEOGRAPH_INC = 1

MODULE = audio_drv
SSPDMA = ssp_dma
CODEC = codec
RESAMPLE = resample
NR = nr

SDK_MODULE_SRC := $(SDK_SRC)module/
STRIPFLAGS = -g --strip-unneeded
STRIP = $(CROSS_COMPILE)strip

ifdef VIDEOGRAPH_INC
EXTRA_CFLAGS += -I$(SDK_MODULE_SRC)/include/videograph.include/log -I$(SDK_MODULE_SRC)/include/videograph.include/em
EXTRA_CFLAGS += -DVIDEOGRAPH_INC=1
endif

ifeq ($(CONFIG_FC7500), y)
EXTRA_CFLAGS += -I$(SDK_MODULE_SRC)/include/cpu_comm
else
EXTRA_CFLAGS += -I$(SDK_MODULE_SRC)/include/frammap
EXTRA_CFLAGS += -I$(SDK_MODULE_SRC)/$(MODULE)/$(SSPDMA) -I$(SDK_MODULE_SRC)/$(MODULE)/$(RESAMPLE) -I$(SDK_MODULE_SRC)/$(MODULE)/$(CODEC) -I$(SDK_MODULE_SRC)/$(MODULE)/$(NR)
endif

ifeq ($(CONFIG_PLATFORM_GM8139), y)
EXTRA_CFLAGS += -I$(SDK_MODULE_SRC)/include/adda302
endif

ifeq ($(CONFIG_PLATFORM_GM8136), y)
EXTRA_CFLAGS += -I$(SDK_MODULE_SRC)/include/adda308 
endif

EXTRA_CFLAGS += -I$(SDK_MODULE_SRC)/include/$(MODULE) -Werror

ifneq ($(KERNELRELEASE),)

$(MODULE)-y := audio_vg.o audio_proc.o

ifneq ($(CONFIG_FC7500), y)
$(MODULE)-y += audio_flow.o $(RESAMPLE)/$(RESAMPLE).o $(SSPDMA)/$(SSPDMA).o $(NR)/fxpt_fft.o
obj-m += $(CODEC)/
endif

obj-m += $(MODULE).o
obj-m := $(sort $(obj-m))

else

modules:
	$(MAKE) -C $(LINUX_SRC) SDK_MODULE_SRC=$(SDK_MODULE_SRC) M=$(PWD) modules
	find -name \*.ko | xargs $(STRIP) $(STRIPFLAGS)

.PHONEY: clean
clean:
	$(MAKE) -C $(LINUX_SRC) M=$(PWD) clean

.PHONEY: all
all: modules

copy:
	cp -f $(MODULE).ko $(SDK_SRC)/target/rootfs-cpio/lib/modules/

endif
