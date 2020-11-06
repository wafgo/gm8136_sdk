sinclude $(PWD)/../../module_conf
SDK_MODULE_SRC := $(SDK_SRC)module/
LINUX_SRC=$(SDK_SRC)linux-3.3-fa
#CROSS_COMPILE=arm-unknown-linux-uclibcgnueabi-

PWD := $(shell pwd)

STRIPFLAGS = -g --strip-unneeded
STRIP = $(CROSS_COMPILE)strip

ifneq ($(KERNELRELEASE),)
EXTRA_CFLAGS+= -Wall -g -I$(SDK_SRC)/module/SSP_ALSA128 -I$(SDK_SRC)/module/SSP_ALSA128/codec

snd_fi2s_adda300_c6-objs:= fi2s-adda300-c6.o adda300.o
snd_fi2s_adda300_c7-objs:= fi2s-adda300-c7.o adda300.o
snd_fi2s_adda300_c3-objs:= fi2s-adda300-c3.o adda300.o
snd_fi2s_nvp1918_c0-objs:= fi2s-nvp1918-c0.o
snd_fi2s_nvp1918_c1-objs:= fi2s-nvp1918-c1.o
snd_fi2s_nvp1118_c0-objs:= fi2s-nvp1118-c0.o
snd_fi2s_nvp1118_c1-objs:= fi2s-nvp1118-c1.o
snd_fi2s_tw2968_c0-objs:= fi2s-tw2968-c0.o
snd_fi2s_tw2964_c0-objs:= fi2s-tw2964-c0.o
snd_fi2s_tw2968_c1-objs:= fi2s-tw2968-c1.o
snd_fi2s_sli10121a_c3-objs:= fi2s-sli10121a-c3.o adda300.o
snd_fi2s_sli10121a_c2-objs:= fi2s-sli10121a-c2.o

modules-y$(CONFIG_PLATFORM_GM8210) += snd_fi2s_adda300_c6.o
modules-y$(CONFIG_PLATFORM_GM8210) += snd_fi2s_adda300_c7.o
modules-y$(CONFIG_PLATFORM_GM8210) += snd_fi2s_adda300_c3.o
modules-y$(CONFIG_PLATFORM_GM8210) += snd_fi2s_nvp1918_c0.o
modules-y$(CONFIG_PLATFORM_GM8210) += snd_fi2s_nvp1918_c1.o
modules-y$(CONFIG_PLATFORM_GM8210) += snd_fi2s_nvp1118_c0.o
modules-y$(CONFIG_PLATFORM_GM8210) += snd_fi2s_nvp1118_c1.o
modules-y$(CONFIG_PLATFORM_GM8210) += snd_fi2s_tw2968_c0.o
modules-y$(CONFIG_PLATFORM_GM8210) += snd_fi2s_tw2968_c1.o
modules-y$(CONFIG_PLATFORM_GM8210) += snd_fi2s_sli10121a_c3.o

modules-y$(CONFIG_PLATFORM_GM8287) += snd_fi2s_nvp1918_c0.o
modules-y$(CONFIG_PLATFORM_GM8287) += snd_fi2s_nvp1118_c0.o
modules-y$(CONFIG_PLATFORM_GM8287) += snd_fi2s_sli10121a_c2.o
modules-y$(CONFIG_PLATFORM_GM8287) += snd_fi2s_tw2964_c0.o


obj-m += $(modules-yy)
obj-m := $(sort $(obj-m))

else


modules:
	$(MAKE) -C $(LINUX_SRC) SDK_MODULE_SRC=$(SDK_MODULE_SRC) M=$(PWD) modules
	@$(STRIP) $(STRIPFLAGS) *.ko

.PHONEY: clean
clean:
	$(MAKE) -C $(LINUX_SRC) M=$(PWD) clean

.PHONEY: all
all: modules

copy:
	cp -f $(MODULE).ko $(SDK_SRC)/target/rootfs-cpio/lib/modules/

endif
