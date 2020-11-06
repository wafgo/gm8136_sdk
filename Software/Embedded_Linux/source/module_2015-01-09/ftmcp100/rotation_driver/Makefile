ifeq ($(KERNELRELEASE),)
PWD := $(shell pwd)
sinclude $(PWD)/../../module_conf
endif

ifeq ($(obj),)
obj=$(PWD)
endif

SDK_MODULE_SRC := $(SDK_SRC)module/

#EXTRA_CFLAGS+=-DDMAWRP

#SUPPORT_VG_IOCTL=VG
#ifeq ($(srctree),)
#include $(LINUX_SRC)/.config
#else
#include $(srctree)/.config
#endif

MODULE = frotation_drv

include $(obj)/../Makefile.rules
include $(obj)/../config.mak

#ifeq ($(SUPPORT_VG_IOCTL),VG)
#EXTRA_CFLAGS+=-I$(SDK_MODULE_SRC)/include/videograph.include/log -I$(SDK_MODULE_SRC)/include/videograph.include/em
#EXTRA_CFLAGS+=-DSUPPORT_VG
#endif

#EXTRA_CFLAGS+=-I$(SDK_MODULE_SRC)/include/frammap -I$(SDK_MODULE_SRC)/include/ftmcp100
#EXTRA_CFLAGS+=-DFPGA8210 -DLINUX
#EXTRA_CFLAGS+=-DTWO_P_EXT

#include $(obj)/../userconfig.mak
#include $(obj)/../Makefile.rules

#common-objs :=  common/jutils.o
#encoder-objs :=  encoder/jcapistd.o encoder/jccoefct.o encoder/jchuff.o encoder/jcmarker.o encoder/jcomapi.o encoder/jcparam.o encoder/jpeg_enc.o
#decoder-objs :=  decoder/ftmcp100.o decoder/jdapimin.o decoder/jdcoefct.o decoder/jdhuff.o decoder/jdinput.o decoder/jdmarker.o decoder/jpeg_dec.o
ifeq ($(SUPPORT_VG_IOCTL),VG)
top-objs := rotation_vg2.o rotation_proc.o rotation_host.o
else
#top-objs := fmjpeg.o fmjpeg_dec.o fmjpeg_enc.o
endif
#top-objs += $(common-objs)
#top-objs += $(encoder-objs)
#top-objs += $(decoder-objs)
$(MODULE)-objs := $(top-objs)
modules-yy := $(MODULE).o

#EXTRA_CFLAGS += -DMJPEG_MODE


ifneq ($(KERNELRELEASE),)
EXTRA_CFLAGS+=-I$(obj)/../
obj-m += $(modules-yy)
obj-m := $(sort $(obj-m))
else
#include $(obj)/config.mak
modules:
	$(MAKE) -C $(LINUX_SRC) SDK_MODULE_SRC=$(SDK_MODULE_SRC) SDK_MODULE_INC=$(SDK_MODULE_INC) LINUX_SRC=$(LINUX_SRC) M=$(PWD) modules
	$(STRIP) $(STRIPFLAGS) $(MODULE).ko

clean:
	$(MAKE) -C $(LINUX_SRC) M=$(PWD) clean

endif

