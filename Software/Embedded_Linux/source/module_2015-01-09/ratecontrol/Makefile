sinclude $(PWD)/../module_conf
PWD := $(shell pwd)

GMGraphInterface=yes
MPEG4_RC_ENABLE=yes
MJPEG_RC_ENABLE=yes
FAVC_210_RC=yes

MODULE = favc_rc
ifeq ($(MPEG4_RC_ENABLE),yes)
MPEG4_MODULE = mp4e_rc
endif
ifeq ($(MJPEG_RC_ENABLE),yes)
MJPEG_MODULE = mje_rc
endif

ifeq ($(obj),)
obj=$(PWD)
endif

include $(obj)/config.mak

SDK_MODULE_SRC := $(SDK_SRC)module/

ifeq ($(FAVC_210_RC),yes)
rc-objs := h264e_ratecontrol_210.o favc_rc_driver_210.o
else
rc-objs := h264e_ratecontrol.o favc_rc_driver.o
endif
ifeq ($(MPEG4_RC_ENABLE),yes)
mpeg4-objs := mp4e_ratecontrol.o mp4e_rc_driver.o
endif
ifeq ($(MJPEG_RC_ENABLE),yes)
mjpeg-objs := mje_ratecontrol.o mje_rc_driver.o
endif

ifneq ($(KERNELRELEASE),)
top-objs += $(rc-objs)

$(MODULE)-objs := $(top-objs)
modules-yy := $(MODULE).o

ifeq ($(MPEG4_RC_ENABLE),yes)
top2-objs += $(mpeg4-objs)
$(MPEG4_MODULE)-objs := $(top2-objs)
modules2-yy := $(MPEG4_MODULE).o
EXTRA_CFLAGS+=-I$(SDK_MODULE_SRC)/include/ftmcp100
endif

ifeq ($(MJPEG_RC_ENABLE),yes)
top3-objs += $(mjpeg-objs)
$(MJPEG_MODULE)-objs := $(top3-objs)
modules3-yy := $(MJPEG_MODULE).o
EXTRA_CFLAGS+=-I$(SDK_MODULE_SRC)/include/ftmcp100
endif

ifeq ($(GMGraphInterface),yes)
EXTRA_CFLAGS+=-DVG_INTERFACE
EXTRA_CFLAGS+=-I$(SDK_MODULE_SRC)/include/videograph.include/em -I$(SDK_MODULE_SRC)/include/videograph.include/log
endif
#EXTRA_CFLAGS+=-DFPGA_8210 -DLINUX -DGM8210
EXTRA_CFLAGS+=-DLINUX
EXTRA_CFLAGS+=-I$(SDK_MODULE_SRC)/include/ftmcp280

obj-m += $(modules-yy)
ifeq ($(MPEG4_RC_ENABLE),yes)
obj-m += $(modules2-yy)
endif
ifeq ($(MJPEG_RC_ENABLE),yes)
obj-m += $(modules3-yy)
endif
obj-m := $(sort $(obj-m))

else

modules:
	$(MAKE) -C $(LINUX_SRC) SDK_MODULE_SRC=$(SDK_MODULE_SRC) SDK_MODULE_INC=$(SDK_MODULE_INC) LINUX_SRC=$(LINUX_SRC) M=$(PWD) modules
	$(MAKE) -C $(LINUX_SRC) M=$(PWD) modules
	$(STRIP) $(STRIPFLAGS) $(MODULE).ko
ifeq ($(MPEG4_RC_ENABLE),yes)
	$(STRIP) $(STRIPFLAGS) $(MPEG4_MODULE).ko
endif
ifeq ($(MJPEG_RC_ENABLE),yes)
	$(STRIP) $(STRIPFLAGS) $(MJPEG_MODULE).ko
endif

clean:
	$(MAKE) -C $(LINUX_SRC) M=$(PWD) clean

copy:
ifeq ($(GMGraphInterface),yes)
	cp $(MODULE).ko /data/test_bench/platform8210/module/
else
	cp $(MODULE).ko /data/mcp280/
endif

endif
