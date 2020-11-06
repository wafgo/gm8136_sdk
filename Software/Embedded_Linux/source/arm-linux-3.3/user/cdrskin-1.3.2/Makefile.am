
# ts A90315 : LIBBURNIA_PKGCONFDIR is defined OS specific in acinclude.m4
# was: pkgconfigdir=$(libdir)/pkgconfig
pkgconfigdir=$(LIBBURNIA_PKGCONFDIR)

libincludedir=$(includedir)/libburn

lib_LTLIBRARIES = libburn/libburn.la 
ACLOCAL_AMFLAGS = -I ./

## ========================================================================= ##

# Build libraries
libburn_libburn_la_LDFLAGS = \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)
# This causes undesired .o names
# configure.ac appends -D options to variable CFLAG
### libburn_libburn_la_CFLAGS = $(LIBBURN_DVD_OBS_64K)
libburn_libburn_la_LIBADD = $(LIBBURN_ARCH_LIBS) $(THREAD_LIBS)
libburn_libburn_la_SOURCES = \
	libburn/async.c \
	libburn/async.h \
	libburn/back_hacks.h \
	libburn/cdtext.c \
	libburn/cleanup.c \
	libburn/cleanup.h \
	libburn/crc.c \
	libburn/crc.h \
	libburn/debug.c \
	libburn/debug.h \
	libburn/drive.c \
	libburn/drive.h \
	libburn/ecma130ab.c \
	libburn/ecma130ab.h \
	libburn/error.h \
	libburn/file.c \
	libburn/file.h \
	libburn/init.c \
	libburn/init.h \
	libburn/libburn.h \
	libburn/libdax_audioxtr.h \
	libburn/libdax_audioxtr.c \
	libburn/libdax_msgs.h \
	libburn/libdax_msgs.c \
	libburn/mmc.c \
	libburn/mmc.h \
	libburn/null.c \
	libburn/null.h \
	libburn/options.c \
	libburn/options.h \
	libburn/os.h \
	libburn/read.c \
	libburn/read.h \
	libburn/sbc.c \
	libburn/sbc.h \
	libburn/sector.c \
	libburn/sector.h \
	libburn/sg.c \
	libburn/sg.h \
	libburn/source.h \
	libburn/source.c \
	libburn/spc.c \
	libburn/spc.h \
	libburn/structure.c \
	libburn/structure.h \
	libburn/toc.c \
	libburn/toc.h \
	libburn/transport.h \
	libburn/util.c \
	libburn/util.h \
	libburn/write.c \
	libburn/write.h

##	libburn/sg-@ARCH@.c \

libinclude_HEADERS = \
	libburn/libburn.h 

install-exec-hook:
	$(LIBBURNIA_LDCONFIG_CMD) "$(DESTDIR)$(libdir)" || echo 'NOTE: Explicite dynamic library configuration failed. If needed, configure manually for:' "$(DESTDIR)$(libdir)"

## ========================================================================= ##

## Build test applications
noinst_PROGRAMS = \
	test/libburner \
	test/offst_source \
	test/telltoc \
	test/dewav \
	test/fake_au \
	test/poll \
	test/structest

bin_PROGRAMS = \
	cdrskin/cdrskin

LIBBURN_EXTRALIBS = $(LIBBURN_ARCH_LIBS) $(THREAD_LIBS)

test_libburner_CPPFLAGS = -Ilibburn
test_libburner_LDADD = $(libburn_libburn_la_OBJECTS) $(LIBBURN_EXTRALIBS)
test_libburner_SOURCES = test/libburner.c
test_offst_source_CPPFLAGS = -Ilibburn
test_offst_source_LDADD = $(libburn_libburn_la_OBJECTS) $(LIBBURN_EXTRALIBS)
test_offst_source_SOURCES = test/offst_source.c
test_telltoc_CPPFLAGS = -Ilibburn
test_telltoc_LDADD = $(libburn_libburn_la_OBJECTS) $(LIBBURN_EXTRALIBS)
test_telltoc_SOURCES = test/telltoc.c
test_dewav_CPPFLAGS = -Ilibburn
test_dewav_LDADD = $(libburn_libburn_la_OBJECTS) $(LIBBURN_EXTRALIBS)
test_dewav_SOURCES = test/dewav.c
test_fake_au_CPPFLAGS = 
test_fake_au_LDADD =
test_fake_au_SOURCES = test/fake_au.c
test_poll_CPPFLAGS = -Ilibburn
test_poll_LDADD = $(libburn_libburn_la_OBJECTS) $(LIBBURN_EXTRALIBS)
test_poll_SOURCES = test/poll.c
test_structest_CPPFLAGS = -Ilibburn
test_structest_LDADD = $(libburn_libburn_la_OBJECTS) $(LIBBURN_EXTRALIBS)
test_structest_SOURCES = test/structest.c

##  cdrskin construction site - ts A60816 - B30807
cdrskin_cdrskin_CPPFLAGS = -Ilibburn
cdrskin_cdrskin_CFLAGS = -DCdrskin_libburn_1_3_2

# cdrskin_cdrskin_LDADD = $(libburn_libburn_la_OBJECTS) $(LIBBURN_EXTRALIBS)
# ts A80123, change proposed by Simon Huggins to cause dynamic libburn linking
cdrskin_cdrskin_LDADD = libburn/libburn.la $(LIBBURN_EXTRALIBS)

cdrskin_cdrskin_SOURCES = cdrskin/cdrskin.c cdrskin/cdrfifo.c cdrskin/cdrfifo.h cdrskin/cdrskin_timestamp.h
##
## Open questions: how to compute $timestamp and express -DX="$timestamp"
##                 

# "make clean" shall remove a few stubborn .libs directories
# which George Danchev reported Dec 03 2011.
# Learned from: http://www.gnu.org/software/automake/manual/automake.html#Clean
clean-local:
	-rm -rf cdrskin/.libs test/.libs


## ========================================================================= ##

## Build documentation (You need Doxygen for this to work)
webhost = http://libburn-api.pykix.org
webpath = /
docdir = $(DESTDIR)$(prefix)/share/doc/$(PACKAGE)-$(VERSION)

doc:	doc/html

doc/html: doc/doxygen.conf
	if [ -f ./doc/doc.lock ]; then \
		$(RM) -r doc/html; \
		doxygen doc/doxygen.conf; \
	fi

doc-upload: doc/html
	scp -r $</* $(webhost):$(webpath)

## ts B00729
## Not by default any more.
## It is unclear who is supposed to create file ./doc/doc.lock
# all: doc

install-data-local: 
	if [ -f ./doc/doc.lock ]; then \
		$(mkinstalldirs) $(docdir)/html; \
		$(INSTALL_DATA) doc/html/* $(docdir)/html; \
	fi

uninstall-local:
	rm -rf $(docdir)

## ========================================================================= ##

# Indent source files
indent_files = \
	$(libburn_libburn_la_SOURCES) \
	$(test_poll_SOURCES) \
	$(test_structest_SOURCES)
	

indent: $(indent_files)
	indent -bad -bap -nbbb -nbbo -nbc -bli0 -br -bls   \
		-cdw -ce -cli0 -ncs -nbfda -i8 -l79 -lc79   \
		-lp -saf -sai -nprs -npsl -saw -sob -ss -ut \
		-sbi0 -nsc -ts8 -npcs -ncdb -fca            \
		$^

.PHONY: indent

## ========================================================================= ##

# Extra things
nodist_pkgconfig_DATA = \
	libburn-1.pc

# http://www.nada.kth.se/cgi-bin/info?(automake.info)Man%20pages
man_MANS = cdrskin/cdrskin.1

EXTRA_DIST = \
	bootstrap \
	libburn-1.pc.in \
	version.h.in \
	doc/comments \
	doc/doxygen.conf.in \
	doc/cookbook.txt \
	doc/mediainfo.txt \
	doc/cdtext.txt \
	README \
	AUTHORS \
	CONTRIBUTORS \
	COPYRIGHT \
	cdrskin/README \
	cdrskin/cdrecord_spy.sh \
	cdrskin/compile_cdrskin.sh \
	cdrskin/convert_man_to_html.sh \
	cdrskin/changelog.txt \
	cdrskin/cdrskin_eng.html \
	cdrskin/wiki_plain.txt \
	cdrskin/cleanup.h \
	cdrskin/cleanup.c \
	libburn/libburn.ver \
	libburn/os-dummy.h \
	libburn/os-freebsd.h \
	libburn/os-linux.h \
	libburn/os-libcdio.h \
	libburn/os-solaris.h \
	libburn/sg-dummy.c \
	libburn/sg-freebsd.c \
	libburn/sg-linux.c \
	libburn/sg-libcdio.c \
	libburn/sg-solaris.c \
	COPYING \
	NEWS \
	ChangeLog \
	INSTALL \
	$(man_MANS)

