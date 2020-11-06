ifeq ($(KERNELRELEASE),)
PWD := $(shell pwd)
sinclude $(PWD)/../module_conf
endif

ifeq ($(obj),)
obj=$(PWD)
endif

include $(obj)/config.mak

SDK_MODULE_SRC := $(SDK_SRC)module/

MODULE = fmcp_drv
include $(obj)/Makefile.rules


top-objs := fmcp.o platform.o fmcp_proc.o
ifeq ($(SHARED_BUFFER_POOL),no)
top-objs += enc_mem_pool.o dec_mem_pool.o mem_pool.o
endif
ifeq ($(ENABLE_CALCULATE_CHECK_SUM),yes)
top-objs += checksum_driver/checkdum_vg.o checksum_driver/checkdum_host.o
endif
$(MODULE)-objs := $(top-objs)
modules-yy := $(MODULE).o

ifneq ($(KERNELRELEASE),)
obj-m += $(modules-yy)
ifeq ($(SUPPORT_FMPEG4),yes)
obj-m += fmpeg4_driver/
EXTRA_CFLAGS+=-DMPEG4_ENABLE
endif
ifeq ($(SUPPORT_JPEG),yes)
obj-m += fmjpeg_driver/
EXTRA_CFLAGS+=-DJPEG_ENABLE
endif
ifeq ($(SUPPORT_ROTATION),yes)
obj-m += rotation_driver/
endif
ifeq ($(SHARED_BUFFER_POOL),yes)
obj-m += mem_pool/
endif
obj-m := $(sort $(obj-m))
else

modules:
	echo $(SDK_MODULE_SRC)
	$(MAKE) -C $(LINUX_SRC) SDK_MODULE_SRC=$(SDK_MODULE_SRC) SDK_MODULE_INC=$(SDK_MODULE_INC) LINUX_SRC=$(LINUX_SRC) M=$(PWD) modules
	$(STRIP) $(STRIPFLAGS) $(MODULE).ko
ifeq ($(SUPPORT_JPEG),yes)
	$(STRIP) $(STRIPFLAGS) fmjpeg_driver/fmjpeg_drv.ko
	mv fmjpeg_driver/fmjpeg_drv.ko .
endif
ifeq ($(SUPPORT_FMPEG4),yes)
	$(STRIP) $(STRIPFLAGS) fmpeg4_driver/fmpeg4_drv.ko
	mv fmpeg4_driver/fmpeg4_drv.ko .
endif
ifeq ($(SUPPORT_ROTATION),yes)
	$(STRIP) $(STRIPFLAGS) rotation_driver/frotation_drv.ko
	mv rotation_driver/frotation_drv.ko .
endif
ifeq ($(SHARED_BUFFER_POOL),yes)
	$(STRIP) $(STRIPFLAGS) mem_pool/pool_drv.ko
	mv mem_pool/pool_drv.ko .
endif

clean:
	$(MAKE) -C $(LINUX_SRC) M=$(PWD) clean
	@rm -f fmcp_drv.*
	
copy:
	cp fmcp_drv.ko /data/test_bench/platform8210/module/
ifeq ($(SUPPORT_JPEG),yes)
	cp fmjpeg_driver/fmjpeg_drv.ko /data/test_bench/platform8210/module/
endif
ifeq ($(SUPPORT_FMPEG4),yes)
	cp fmpeg4_driver/fmpeg4_drv.ko /data/test_bench/platform8210/module/
endif

endif
