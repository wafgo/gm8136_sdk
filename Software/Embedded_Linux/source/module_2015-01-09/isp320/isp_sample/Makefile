sinclude $(PWD)/../../../module/module_conf
sinclude $(LINUX_SRC)cross_compiler_def

CFLAGS += -O2
#CFLAGS += -DDEBUG
#CFLAGS += -DMEASURE_TIME
CC = $(CROSS_COMPILE)gcc
STRIP = $(CROSS_COMPILE)strip
STRIPFLAGS = -g --strip-unneeded

SDK_MODULE_SRC := $(SDK_SRC)module
SDK_MODULE_INCDIR := $(SDK_MODULE_SRC)/include

CFLAGS += -Wall
CFLAGS += -I$(SDK_MODULE_SRC)/include/isp320
PROGS = isp_sample
OBJS := isp_sample.o

$(PROGS): $(OBJS)
	$(CC) -o $(PROGS) $(OBJS)
ifeq ($(COMPILE_PC), N)
	@$(STRIP) $(STRIPFLAGS) $(PROGS)
endif
	@cp $(PROGS) ../../../target/rootfs-cpio/mnt/mtd

%: %.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	@rm -f $(PROGS)
	@find . \( -name '*.[oas]' -o -name '*.ko' -o -name '.*.cmd' -o -name '*~' \
        -o -name '.*.d' -o -name '.*.tmp' -o -name '*.mod.c' \) \
        -type f -print | xargs rm -f
