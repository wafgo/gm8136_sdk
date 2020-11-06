sinclude $(PWD)/../module_conf

SDK_MODULE_SRC := $(SDK_SRC)module/
STRIPFLAGS = -g --strip-unneeded
STRIP = $(CROSS_COMPILE)strip

EXTRA_CFLAGS+=-I$(SDK_MODULE_SRC)/cpu_comm_v2/include -I$(SDK_MODULE_SRC)/include -I$(LINUX_SRC) -Wall

ifneq ($(KERNELRELEASE),)

platform-$(CONFIG_PLATFORM_GM8126) := plat_gm8126.o
platform-$(CONFIG_PLATFORM_GM8210) := plat_gm8210.o


cpu_comm_fa626-objs := common/cpu_comm_if.o common/zebra.o common/mem_alloc.o common/proc.o common/dma_llp.o common/$(platform-y)
cpu_comm_fa726-objs := common/cpu_comm_if.o common/zebra.o common/mem_alloc.o common/proc.o common/dma_llp.o common/$(platform-y)
fa726_lcd-objs := fa726/lcd/framebuffer.o fa726/lcd/proc.o

sample-objs := sample_code.o

modules-yy += cpu_comm_fa626.o
modules-yy += cpu_comm_fa726.o
modules-yy += fa726_lcd.o

obj-m += $(modules-yy)
obj-m := $(sort $(obj-m))

else

PWD := $(shell pwd)

modules:
	$(MAKE) -C $(LINUX_SRC) M=$(PWD) modules
	find -name \*.ko | xargs $(STRIP) $(STRIPFLAGS)

.PHONEY: clean
clean:
	$(MAKE) -C $(LINUX_SRC) M=$(PWD) clean

endif





