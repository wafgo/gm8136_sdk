ifeq ($(KERNELRELEASE),)
PWD := $(shell pwd)
sinclude $(PWD)/../../module_conf
endif

ifeq ($(obj),)
obj=$(PWD)
endif

SDK_MODULE_SRC := $(SDK_SRC)module/

EXTRA_CFLAGS+=-DDMAWRP

#SUPPORT_VG_IOCTL=VG
#ifeq ($(srctree),)
#include $(LINUX_SRC)/.config
#else
#include $(srctree)/.config
#endif
MODULE = fmpeg4_drv

#ifeq ($(SUPPORT_VG_IOCTL),VG)
#EXTRA_CFLAGS+=-I$(SDK_MODULE_SRC)/include/videograph.include/log -I$(SDK_MODULE_SRC)/include/videograph.include/em
#EXTRA_CFLAGS+=-DSUPPORT_VG
#endif

#EXTRA_CFLAGS+=-I$(SDK_MODULE_SRC)/include/frammap -I$(SDK_MODULE_SRC)/include/ftmcp100
#EXTRA_CFLAGS+=-DFPGA8210 -DLINUX
#EXTRA_CFLAGS+=-DTWO_P_EXT

EXTRA_CFLAGS+=-I$(SDK_MODULE_SRC)/ftmcp100/fmpeg4_driver
EXTRA_CFLAGS+=-I$(SDK_MODULE_SRC)/ftmcp100/fmpeg4_driver/mpeg4_decoder -I$(SDK_MODULE_SRC)/ftmcp100/fmpeg4_driver/mpeg4_encoder

#include $(obj)/../userconfig.mak
include $(obj)/../Makefile.rules

common-objs :=  common/quant_matrix.o common/zigzag.o common/image.o
encoder-objs :=  mpeg4_encoder/bitstream.o mpeg4_encoder/dma_e.o mpeg4_encoder/encoder_host.o mpeg4_encoder/image_e.o mpeg4_encoder/me_e.o mpeg4_encoder/Mp4Venc.o
decoder-objs :=  mpeg4_decoder/bitstream_d.o mpeg4_decoder/dma_b.o mpeg4_decoder/decoder_host.o mpeg4_decoder/image_d.o mpeg4_decoder/me.o mpeg4_decoder/Mp4Vdec.o
ifeq ($(CONFIG_FTMCP100_CORE),EMBEDDED)
encoder-objs += mpeg4_encoder/encoder_ext.o
decoder-objs += mpeg4_decoder/decoder_ext.o
else
encoder-objs += mpeg4_encoder/encoder.o
decoder-objs += mpeg4_decoder/decoder.o
endif


ifeq ($(SUPPORT_VG_IOCTL),VG)
top-objs := fmpeg4.o fmpeg4_enc_vg2.o fmpeg4_enc_proc.o fmpeg4_dec_vg2.o fmpeg4_dec_proc.o
#top-objs := fmpeg4.o fmpeg4_enc_vg2.o fmpeg4_enc_proc.o
else
top-objs := fmpeg4.o fmpeg4_dec.o fmpeg4_enc.o
#top-objs := fmpeg4.o fmpeg4_enc.o
endif

ifeq ($(MPEG4_CUSTOMER_QUANT_TABLE_SUPPORT), yes)
EXTRA_CFLAGS+= -DCUSTOMER_QUANT_MATRIX
endif
EXTRA_CFLAGS+= -DFPGA


top-objs += $(common-objs)
top-objs += $(encoder-objs)
top-objs += $(decoder-objs)
$(MODULE)-objs := $(top-objs)
modules-yy := $(MODULE).o


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

