sinclude $(PWD)/../../module_conf

SDK_MODULE_SRC := $(SDK_SRC)module/

EXTRA_CFLAGS+=-I$(SDK_MODULE_SRC)/include/

ifneq ($(KERNELRELEASE),)
test_host_comm-objs := test_host.o
test_device_comm-objs := test_device.o

pci_test_host_comm-objs := pci_test_host.o
pci_test_device_comm-objs := pci_test_device.o


modules-yy += test_host_comm.o
modules-yy += test_device_comm.o

modules-yy += pci_test_host_comm.o
modules-yy += pci_test_device_comm.o


obj-m += $(modules-yy)
obj-m := $(sort $(obj-m))

else

PWD := $(shell pwd)

modules:
	$(MAKE) -C $(LINUX_SRC) M=$(PWD) modules

clean:
	$(MAKE) -C $(LINUX_SRC) M=$(PWD) clean
endif

