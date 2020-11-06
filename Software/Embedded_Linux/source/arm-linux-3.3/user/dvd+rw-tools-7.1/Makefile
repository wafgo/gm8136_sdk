all:
	@(m4 -DOS=`uname -s` Makefile.m4 | $(MAKE) -f - dvd+rw-tools)
install:# BSD make doesn't support wild-card targets:-(
	@(m4 -DOS=`uname -s` Makefile.m4 | $(MAKE) -f - $@)
nothing:# dumb target
.%:	# don't mess with Solaris .INIT/.DONE, ...
%:	# ... but the rest just passes through
	@(m4 -DOS=`uname -s` Makefile.m4 | $(MAKE) -f - $@)

CHAIN=growisofs dvd+rw-format dvd+rw-booktype dvd+rw-mediainfo dvd-ram-control
clean:
	-(rm *.o $(CHAIN) rpl8 btcflash panaflash; exit 0) < /dev/null > /dev/null 2>&1

VER=7.1
DIST=dvd+rw-tools-$(VER)

pkg:
	[ -h $(DIST) ] || ln -s . $(DIST)
	tar chf dvd+rw-tools-$(VER).tar		\
		--owner=bin --group=bin 	\
		$(DIST)/Makefile		\
		$(DIST)/Makefile.m4		\
		$(DIST)/dvd+rw-tools.spec	\
		$(DIST)/growisofs.1		\
		$(DIST)/transport.hxx		\
		$(DIST)/mp.h			\
		$(DIST)/win32err.h		\
		$(DIST)/keys.txt		\
		$(DIST)/genasctable.pl		\
		$(DIST)/asctable.h		\
		$(DIST)/growisofs.c		\
		$(DIST)/growisofs_mmc.cpp	\
		$(DIST)/dvd+rw-format.cpp	\
		$(DIST)/dvd+rw-mediainfo.cpp	\
		$(DIST)/dvd+rw-booktype.cpp	\
		$(DIST)/dvd-ram-control.cpp	\
		$(DIST)/rpl8.cpp		\
		$(DIST)/btcflash.cpp		\
		$(DIST)/LICENSE
	if [ -f index.html ]; then		\
	    tar rhf dvd+rw-tools-$(VER).tar	\
		--owner=bin --group=bin		\
		$(DIST)/index.html;		\
	fi
	gzip -f dvd+rw-tools-$(VER).tar
	rm $(DIST)
