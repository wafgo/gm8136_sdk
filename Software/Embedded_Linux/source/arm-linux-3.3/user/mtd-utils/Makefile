
# -*- sh -*-

PREFIX=/usr
EXEC_PREFIX=$(PREFIX)
SBINDIR=$(EXEC_PREFIX)/sbin
MANDIR=$(PREFIX)/share/man
INCLUDEDIR=$(PREFIX)/include
ifeq ($(x86), Y)
CROSS= 
DEPDIR=/usr/src/arm-linux-3.3/user/dep/x86_32
else
#CROSS=arm-none-linux-gnueabi-
CROSS=arm-unknown-linux-uclibcgnueabi-
DEPDIR=/usr/src/arm-linux-3.3/user/dep/arm
endif
CC := $(CROSS)gcc
STRIP=$(CROSS)strip

#GM add dependency header and libs
DEPINCLUDEDIR=$(DEPDIR)/include/
ZLIBCPPFLAGS=-I$(DEPINCLUDEDIR)
LZOCPPFLAGS=-I$(DEPINCLUDEDIR)
ACLCPPFLAGS=-I$(DEPINCLUDEDIR)
LDLIBS += -static -L$(DEPDIR)/lib -lz -llzo2 

CFLAGS ?= -O2 
CFLAGS += -Wall #-Werror 
CPPFLAGS += -I./include $(ZLIBCPPFLAGS) $(LZOCPPFLAGS) $(ACLCPPFLAGS)
ifeq ($(origin CROSS),undefined)
  BUILDDIR := ./host
else
# Remove the trailing slash to make the directory name
ifeq ($(x86), Y)
  BUILDDIR := ./x86
else
  BUILDDIR := $(CROSS:-=)
endif
endif

ifeq ($(WITHOUT_XATTR), 1)
  CPPFLAGS += -DWITHOUT_XATTR
endif

#RAWTARGETS = ftl_format flash_erase flash_eraseall nanddump doc_loadbios \
#	ftl_check mkfs.jffs2 flash_lock flash_unlock flash_info \
#	flash_otp_info flash_otp_dump mtd_debug flashcp nandwrite nandtest \
#	jffs2dump \
#	nftldump nftl_format docfdisk \
#	rfddump rfdformat \
#	serve_image recv_image \
#	sumtool #jffs2reader
RAWTARGETS = mkfs.jffs2 ftl_format flash_erase flash_eraseall flashcp nanddump nandwrite nandtest flash_info \
	gm_spi_write 

TARGETS = $(foreach target,$(RAWTARGETS),$(BUILDDIR)/$(target))

SYMLINKS =

%: %.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^
	$(STRIP) $@

$(BUILDDIR)/%.o: %.c
	mkdir -p $(BUILDDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $< -Wp,-MD,$(BUILDDIR)/.$(<F).dep

.SUFFIXES:

all: $(TARGETS)
	#$(MAKE) -C $(BUILDDIR)/ubi-utils
	#$(MAKE) -C $(BUILDDIR)/mkfs.ubifs

IGNORE=${wildcard $(BUILDDIR)/.*.c.dep}
-include ${IGNORE}

clean:
	rm -f $(BUILDDIR)/*.o $(TARGETS) $(BUILDDIR)/.*.c.dep $(SYMLINKS)
	if [ "$(BUILDDIR)x" != ".x" ]; then rm -rf $(BUILDDIR); fi
	#$(MAKE) -C $(BUILDDIR)/ubi-utils clean
	#$(MAKE) -C $(BUILDDIR)/mkfs.ubifs clean

$(SYMLINKS):
	ln -sf ../fs/jffs2/$@ $@

$(BUILDDIR)/mkfs.jffs2: $(BUILDDIR)/crc32.o \
			$(BUILDDIR)/compr_rtime.o \
			$(BUILDDIR)/mkfs.jffs2.o \
			$(BUILDDIR)/compr_zlib.o \
			$(BUILDDIR)/compr_lzo.o \
			$(BUILDDIR)/compr.o \
			$(BUILDDIR)/rbtree.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS) $(ZLIBLDFLAGS) -lz $(LZOLDFLAGS) -llzo2

$(BUILDDIR)/flash_eraseall: $(BUILDDIR)/crc32.o $(BUILDDIR)/flash_eraseall.o
	$(CC) $(LDFLAGS) -o $@ $^
	$(STRIP) $@

$(BUILDDIR)/jffs2reader: $(BUILDDIR)/jffs2reader.o
	$(CC) $(LDFLAGS) -o $@ $^ $(ZLIBLDFLAGS) -lz
	$(STRIP) $@

$(BUILDDIR)/jffs2dump: $(BUILDDIR)/jffs2dump.o $(BUILDDIR)/crc32.o
	$(CC) $(LDFLAGS) -o $@ $^
	$(STRIP) $@

$(BUILDDIR)/sumtool: $(BUILDDIR)/sumtool.o $(BUILDDIR)/crc32.o
	$(CC) $(LDFLAGS) -o $@ $^
	$(STRIP) $@

$(BUILDDIR)/serve_image: $(BUILDDIR)/serve_image.o $(BUILDDIR)/crc32.o $(BUILDDIR)/fec.o
	$(CC) $(LDFLAGS) -o $@ $^
	$(STRIP) $@

$(BUILDDIR)/recv_image: $(BUILDDIR)/recv_image.o $(BUILDDIR)/crc32.o $(BUILDDIR)/fec.o
	$(CC) $(LDFLAGS) -o $@ $^
	$(STRIP) $@

$(BUILDDIR)/fectest: $(BUILDDIR)/fectest.o $(BUILDDIR)/crc32.o $(BUILDDIR)/fec.o
	$(CC) $(LDFLAGS) -o $@ $^
	$(STRIP) $@

install: ${TARGETS}
	mkdir -p ${DESTDIR}/${SBINDIR}
	install -m0755 ${TARGETS} ${DESTDIR}/${SBINDIR}/
	mkdir -p ${DESTDIR}/${MANDIR}/man1
	gzip -9c mkfs.jffs2.1 > ${DESTDIR}/${MANDIR}/man1/mkfs.jffs2.1.gz
	#$(MAKE) -C $(BUILDDIR)/ubi-utils install
	#$(MAKE) -C $(BUILDDIR)/mkfs.ubifs install
