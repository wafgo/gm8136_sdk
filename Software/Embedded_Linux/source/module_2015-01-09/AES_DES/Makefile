sinclude $(PWD)/../module_conf

MODULE = aes_des
ifeq ($(obj),)
obj=$(PWD)
endif

ifndef SDK_MODULE_SRC
VERS2614 = $(findstring arm-linux-2.6,$(obj))
VERS2628 = $(findstring arm-linux-2.6.28,$(obj))
VERS3300 = $(findstring arm-linux-3.3,$(obj))

ifneq ($(VERS3300),)
#SDK_SRC := /usr/src/arm-linux-3.3/
LINUX_SRC = $(SDK_SRC)linux-3.3-fa/
SDK_MODULE_SRC = $(SDK_SRC)module/
SDK_MODULE_INC = $(SDK_MODULE_SRC)include/
EXTRA_CFLAGS += -I$(SDK_MODULE_INC)/security
EXTRA_CFLAGS += -I$(SDK_MODULE_INC)/frammap
else ifneq ($(VERS2628),)
#SDK_SRC := /usr/src/arm-linux-2.6.28/
LINUX_SRC = $(SDK_SRC)linux-2.6.28-fa/
SDK_MODULE_SRC = $(SDK_SRC)module/
SDK_MODULE_INC = $(SDK_MODULE_SRC)include/
EXTRA_CFLAGS += -I$(SDK_MODULE_INC)/security
else
#SDK_SRC := /usr/src/arm-linux-2.6/
LINUX_SRC = $(SDK_SRC)linux-2.6.14-fa/
SDK_MODULE_SRC = $(SDK_SRC)module/
SDK_MODULE_INC = $(SDK_MODULE_SRC)
EXTRA_CFLAGS += -I$(SDK_MODULE_INC)/AES_DES
endif

endif

EXTRA_CFLAGS += -fno-common

STRIPFLAGS = -g --strip-unneeded
STRIP = $(CROSS_COMPILE)strip

ifneq ($(KERNELRELEASE),)
$(MODULE)-objs-yy := security.o

$(MODULE)-objs-y$(CONFIG_PLATFORM_GM8181) += gm8181.o
$(MODULE)-objs-y$(CONFIG_PLATFORM_GM8126) += gm8126.o
$(MODULE)-objs-y$(CONFIG_PLATFORM_GM8210) += gm8210.o
$(MODULE)-objs-y$(CONFIG_PLATFORM_GM8287) += gm8287.o
$(MODULE)-objs-y$(CONFIG_PLATFORM_GM8139) += gm8139.o
$(MODULE)-objs-y$(CONFIG_PLATFORM_GM8136) += gm8136.o

$(MODULE)-objs := $($(MODULE)-objs-yy)

obj-m += $(MODULE).o
obj-m := $(sort $(obj-m))

else

PWD := $(shell pwd)

modules:
	$(MAKE) -C $(LINUX_SRC) M=$(PWD) modules
	@$(STRIP) $(STRIPFLAGS) $(MODULE).ko

clean:
	$(MAKE) -C $(LINUX_SRC) M=$(PWD) clean
endif
