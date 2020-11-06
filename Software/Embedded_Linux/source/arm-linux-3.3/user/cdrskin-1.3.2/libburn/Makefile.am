pkgconfigdir=$(libdir)/pkgconfig
libincludedir=$(includedir)/libburn

lib_LTLIBRARIES = libburn.la

libburn_la_SOURCES = \
	async.c \
	async.h \
	crc.c \
	crc.h \
	debug.c \
	debug.h \
	drive.c \
	drive.h \
	file.c \
	file.h \
	init.c \
	init.h \
	lec.c \
	lec.h \
	message.c \
	message.h \
	mmc.c \
	mmc.h \
	null.c \
	null.h \
	options.c \
	options.h \
	read.c \
	read.h \
	sbc.c \
	sbc.h \
	sector.c \
	sector.h \
	sg.c \
	sg.h \
	spc.c \
	spc.h \
	source.h \
	source.c \
	structure.c \
	structure.h \
	toc.c \
	toc.h \
	transport.h \
	util.c \
	util.h \
	write.c \
	write.h

libinclude_HEADERS = libburn.h

## ========================================================================= ##
indent_files = $(libburn_la_SOURCES) 

indent: $(indent_files)
	indent -bad -bap -nbbb -nbbo -nbc -bli0 -br -bls   \
	       -cdw -ce -cli0 -ncs -nbfda -i8 -l79 -lc79   \
	       -lp -saf -sai -nprs -npsl -saw -sob -ss -ut \
	       -sbi0 -nsc -ts8 -npcs -ncdb -fca            \
	       $^

.PHONY: indent

## ========================================================================= ##
