# OBS! M4 processed!
changequote([, ])
[
CHAIN=growisofs dvd+rw-format dvd+rw-booktype dvd+rw-mediainfo dvd-ram-control

dvd+rw-tools:	$(CHAIN)

WARN=#-Wall	# developers are welcomed to build with `make WARN=-Wall'
]

# fix-up OS macro
ifelse(substr(OS,0,7),[CYGWIN_],[define([OS],[MINGW32])])
ifelse(substr(OS,0,7),[MINGW32],[define([OS],[MINGW32])])
ifelse(OS,NetBSD,[define([OS],[BSD])CXXFLAGS+=-D__unix])
ifelse(OS,OpenBSD,[define([OS],[BSD])])
ifelse(OS,FreeBSD,[define([OS],[BSD])LDLIBS=-lcam])
ifelse(OS,IRIX64,[define([OS],[IRIX])])

ifelse(OS,Darwin,[
#
# Mac OS X section
#
CC	=gcc
CFLAGS	+=$(WARN) -D__unix -O2
CXX	=g++
CXXFLAGS+=$(WARN) -D__unix -O2 -fno-exceptions
LDLIBS	=-framework CoreFoundation -framework IOKit
LINK.o	=$(LINK.cc)

# to install set-root-uid, `make BIN_MODE=04755 install'...
BIN_MODE?=0755
install:	dvd+rw-tools
	install -o root -m $(BIN_MODE) $(CHAIN) /usr/bin
	install -o root -m 0644 growisofs.1 /usr/share/man/man1
])

ifelse(OS,MINGW32,[
#
# MINGW section
#
CC	=gcc
CFLAGS	+=$(WARN) -mno-cygwin -O2
CXX	=g++
CXXFLAGS+=$(WARN) -mno-cygwin -O2 -fno-exceptions
LINK.o	=$(LINK.cc)
])

ifelse(OS,BSD,[
#
# OpenBSD/NetBSD/FreeBSD section
#
CC	?=gcc
CFLAGS	+=$(WARN) -O2 -pthread -D_THREAD_SAFE -D_REENTRANT
CXX	?=g++
CXXFLAGS+=$(WARN) -O2 -fno-exceptions -pthread -D_THREAS_SAFE -D_REENTRANT

.SUFFIXES: .c .cpp .o

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<
.cpp.o:
	$(CXX) $(CXXFLAGS) -c -o $@ $<
.o:	# try to please both BSD vv&vv GNU make at the same time...
	$(CXX) $(CXXFLAGS) -o $@ $> $^ $(LDFLAGS) $(LDLIBS)

# yes, default is set-root-uid, `make BIN_MODE=0755 install' to override...
BIN_MODE?=04755
install:	dvd+rw-tools
	install -o root -m $(BIN_MODE) $(CHAIN) /usr/local/bin
	install -o root -m 0644 growisofs.1 /usr/local/man/man1
])

ifelse(OS,SunOS,[
#
# Solaris section
#
.SUFFIXES: .c .cpp .o

# check for WorkShop C++
syscmd([(CC -flags) > /dev/null 2>&1])
ifelse(sysval,0,[
CC	=cc
CFLAGS	=-O -xstrconst -w -D_REENTRANT -D__`uname -s`=`uname -r | tr -d .`
CXX	=CC
CXXFLAGS=-O -features=no%except,conststrings -w -D_REENTRANT
LDFLAGS	=-staticlib=%all
],[
CC	=gcc
CFLAGS	=$(WARN) -O2 -D_REENTRANT -D__`uname -s`=`uname -r | tr -d .`
CXX	=g++
CXXFLAGS=$(WARN) -O2 -fno-exceptions -D_REENTRANT
])

LDLIBS=-lvolmgt -lrt -lpthread -ldl

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<
.cpp.o:
	$(CXX) $(CXXFLAGS) -c -o $@ $<
%:	%.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

install:	dvd+rw-tools
	/usr/ucb/install -o root -m 04755 $(CHAIN) /usr/local/bin
	/usr/ucb/install -o root -m 0644 growisofs.1 /usr/local/man/man1
])

ifelse(OS,HP-UX,[
#
# HP-UX section
#
.SUFFIXES: .c .cpp .o

# check for HP C++
syscmd([(aCC -E /dev/null) > /dev/null 2>&1])
ifelse(sysval,0,[
# run `make TARGET_ARCH=+DD64' for 64-bit build...
# ... or should we check for `getconf KERNEL_BITS' and/or CPU_VERSION?
# syscmd([([ `getconf KERNEL_BITS` -eq 64 ]) > /dev/null 2>&1])
CC      =cc
CFLAGS  =$(TARGET_ARCH) -O -D_REENTRANT
CXX     =aCC
CXXFLAGS=$(TARGET_ARCH) -O +noeh -D_REENTRANT
],[
CC	=gcc
CFLAGS	=$(WARN) -O2 -D_REENTRANT
CXX	=g++
CXXFLAGS=$(WARN) -O2 -fno-exceptions -D_REENTRANT
])
LDLIBS=-lrt -lpthread

.c.o:
	$(CC) $(CFLAGS) \
		-DSCTL_MAJOR=`/usr/sbin/lsdev -h -d sctl | awk '{print$$1}'` \
		-c -o $@ $<
.cpp.o:
	$(CXX) $(CXXFLAGS) -c -o $@ $<
.o:	# try to please both BSD vv&vv GNU make at the same time...
	$(CXX) $(CXXFLAGS) -o $@ $> $^ $(LDFLAGS) $(LDLIBS)

install:	dvd+rw-tools
	/usr/sbin/install -o -f /usr/local/bin  $(CHAIN)
	/usr/sbin/install -o -f /usr/local/man/man1 growisofs.1
])

ifelse(OS,IRIX,[
#
# IRIX section
#
.SUFFIXES: .c .cpp .o

# check for MIPSpro compiler
syscmd([(CC -version) > /dev/null 2>&1])
ifelse(sysval,0,[
CC	=cc
CFLAGS	=$(WARN) -O -use_readonly_const -D_SGI_MP_SOURCE
CXX	=CC
CXXFLAGS=$(WARN) -O -OPT:Olimit=0 -use_readonly_const -LANG:exceptions=OFF -D_SGI_MP_SOURCE

],[
CC	=gcc
CFLAGS	=$(WARN) -O2 -D_SGI_MP_SOURCE
CXX	=g++
CXXFLAGS=$(WARN) -O2 -fno-exceptions -D_SGI_MP_SOURCE
])

LDLIBS=-lmediad -lpthread
LDFLAGS=-dont_warn_unused

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<
.cpp.o:
	$(CXX) $(CXXFLAGS) -c -o $@ $<
.o:
	$(CXX) $(CXXFLAGS) -o $@ $> $^ $(LDFLAGS) $(LDLIBS)

BIN_MODE=04755	# set-root-uid
install:	dvd+rw-tools
	/sbin/install -u root -m $(BIN_MODE) $(CHAIN) /usr/local/bin
	/sbin/install -u root -m 0644 growisofs.1 /usr/local/man/man1
])

ifelse(OS,Linux,[
#
# Linux section
#
CC	=arm-unknown-linux-uclibcgnueabi-gcc
CFLAGS	+=$(WARN) -O2 -D_REENTRANT
CXX	=arm-unknown-linux-uclibcgnueabi-g++
CXXFLAGS+=$(WARN) -O2 -fno-exceptions -D_REENTRANT
LDLIBS	=-lpthread -static
LINK.o	=$(LINK.cc)

prefix?=/usr/local
manprefix?=$(shell case $(prefix) in (*/usr/?*) echo $(prefix)/man ;; (*) echo $(prefix)/share/man ;; esac)

bin_mode?=0755	# yes, default is *no* set-uid
minus_o:=$(shell [[ `id -u` == 0 ]] && echo "-o root")

install:	dvd+rw-tools
	[[ -d $(prefix)/bin ]] || mkdir -p $(prefix)/bin
	install $(minus_o) -m $(bin_mode) $(CHAIN) $(prefix)/bin
	[[ -d $(manprefix)/man1 ]] || mkdir -p $(manprefix)/man1
	install $(minus_o) -m 0644 growisofs.1 $(manprefix)/man1
	-[[ -f rpl8 ]] && install $(minus_o) -m $(bin_mode) rpl8 $(prefix)/bin; :
	-[[ -f btcflash ]] && install $(minus_o) -m $(bin_mode) btcflash $(prefix)/bin; :
])

# common section
[
growisofs:		growisofs_mmc.o growisofs.o
growisofs.o:		growisofs.c mp.h
growisofs_mmc.o:	growisofs_mmc.cpp transport.hxx asctable.h
asctable.h:		keys.txt
	perl genasctable.pl < keys.txt > $@

dvd+rw-format:		dvd+rw-format.o
dvd+rw-format.o:	dvd+rw-format.cpp transport.hxx asctable.h

dvd+rw-mediainfo:	dvd+rw-mediainfo.o
dvd+rw-mediainfo.o:	dvd+rw-mediainfo.cpp transport.hxx asctable.h

dvd+rw-booktype:	dvd+rw-booktype.o
dvd+rw-booktype.o:	dvd+rw-booktype.cpp transport.hxx asctable.h

dvd-ram-control:	dvd-ram-control.o
dvd-ram-control.o:	dvd-ram-control.cpp transport.hxx asctable.h

rpl8:			rpl8.o
rpl8.o:			rpl8.cpp transport.hxx asctable.h
+rpl8:			rpl8
#so that I can invoke `make +rpl8' to build rpl8...
btcflash:		btcflash.o
btcflash.o:		btcflash.cpp transport.hxx asctable.h
+btcflash:		btcflash
#so that I can invoke `make +btcflash' to build btcflash...
panaflash:		panaflash.o
panaflash.o:		panaflash.cpp transport.hxx asctable.h
+panaflash:		panaflash
]
