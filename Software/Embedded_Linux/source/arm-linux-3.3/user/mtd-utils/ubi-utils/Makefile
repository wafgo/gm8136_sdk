#
# Makefile for ubi-utils
#

#CROSS := arm-none-linux-gnueabi-
CROSS := arm-unknown-linux-uclibcgnueabi-
KERNELHDR := ../include
DESTDIR := /usr/local
SBINDIR=/usr/sbin
MANDIR=/usr/share/man
#INCLUDEDIR=/usr/include
#INCLUDEDIR=/opt/crosstool/arm-linux/arm-none-linux-gnueabi-4.4.0_ARMv5TE/arm-none-linux-gnueabi/include

CC := $(CROSS)gcc
#CFLAGS ?= -O2 -g -Werror
CFLAGS ?= -O2 -g 
CFLAGS += -Wall -Wwrite-strings -W -I../tmp/e2fsprogs-1.41.12/build/lib/ -I../tmp/zlib-1.2.5 -I../tmp/lzo-2.04/include/
CPPFLAGS += -I./inc -I./src -I$(KERNELHDR) \
	-std=gnu99 -DPACKAGE_VERSION=\"1.0\"

PERLPROGS = mkpfi ubicrc32.pl

NTARGETS = ubiattach ubicrc32 ubidetach ubimkvol ubinfo ubinize \
	ubirmvol ubiupdatevol ubiformat
TARGETS = pfiflash pddcustomize ubimirror bin2nand nand2bin ubigen \
	mkbootenv unubi pfi2bin $(NTARGETS)

vpath   %.c ./src

%: %.o
	$(CC) $(LDFLAGS) -g -o $@ $^

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $< -g -Wp,-MD,.$(shell basename $<).dep

all: $(TARGETS)

IGNORE=${wildcard .*.c.dep}
-include ${IGNORE}

$(NTARGETS):
	$(MAKE) -C new-utils $@
	mv new-utils/$@ $@

clean:
	rm -rf *.o $(TARGETS) .*.c.dep
	$(MAKE) -C new-utils clean

pddcustomize: pddcustomize.o error.o libubimirror.o bootenv.o hashmap.o \
		libubi.o crc32.o
	$(CC) $(LDFLAGS) -o $@ $^

pfiflash: pfiflash.o libpfiflash.o list.o reader.o error.o libubimirror.o \
		bootenv.o hashmap.o pfi.o libubi.o crc32.o
	$(CC) $(LDFLAGS) -o $@ $^

ubimirror: ubimirror.o error.o libubimirror.o bootenv.o hashmap.o \
		libubi.o crc32.o
	$(CC) $(LDFLAGS) -o $@ $^

nand2bin: nand2bin.o nandecc.o nandcorr.o
	$(CC) $(LDFLAGS) -o $@ $^

bin2nand: bin2nand.o error.o nandecc.o
	$(CC) $(LDFLAGS) -o $@ $^

ubigen: ubigen.o libubigen.o crc32.o
	$(CC) $(LDFLAGS) -o $@ $^

mkbootenv: mkbootenv.o bootenv.o hashmap.o error.o crc32.o
	$(CC) $(LDFLAGS) -o $@ $^

unubi: unubi.o crc32.o unubi_analyze.o eb_chain.o
	$(CC) $(LDFLAGS) -o $@ $^

pfi2bin: pfi2bin.o peb.o error.o list.o crc32.o libubigen.o bootenv.o \
		hashmap.o reader.o pfi.o
	$(CC) $(LDFLAGS) -o $@ $^

install: ${TARGETS}
	mkdir -p ${DESTDIR}/${SBINDIR}
	install -m0755 ${TARGETS} ${DESTDIR}/${SBINDIR}/
	(cd perl && install ${PERLPROGS} ${DESTDIR}/${SBINDIR}/)

uninstall:
	for file in ${TARGETS} ${PERLPROGS}; do \
		$(RM) ${DESTDIR}/${SBINDIR}/$$file; \
	done
