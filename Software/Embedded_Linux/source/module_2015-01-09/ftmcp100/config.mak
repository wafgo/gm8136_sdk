AS	=$(CROSS_COMPILE)as
LD	=$(CROSS_COMPILE)ld
CC	=$(CROSS_COMPILE)gcc
CPP	=$(CC) -E
AR	=$(CROSS_COMPILE)ar
NM	=$(CROSS_COMPILE)nm
STRIP	=$(CROSS_COMPILE)strip
OBJCOPY =$(CROSS_COMPILE)objcopy
OBJDUMP =$(CROSS_COMPILE)objdump
RANLIB	=$(CROSS_COMPILE)RANLIB
STRIPFLAGS = -g --strip-unneeded

%.map:  %.ko
	@$(NM) -n $< | grep -v '\( [aUw] \)\|\(__crc_\)\|\( \$[adt]\)' > $@
#	@$(NM) $< | \
#	grep -v '\(compiled\)\|\(\.o$$\)\|\( [aUw] \)\|\(\.\.ng$$\)\|\(LASH[RL]DI\)' | \
#	sort > $@

%.dis:	%.o
	@$(OBJDUMP) -d $< > $@

.PHONEY: ALL

ALL: modules module.info

module.info:
	$(MAKE) -C $(PWD) debug_info

debug_info: $(patsubst %.o,%.map,$(modules-yy)) $(patsubst %.o,%.dis,$(modules-yy))

clobber: clean
	@find . \( -name '*.[oas]' -o -name '*.ko' -o -name '.*.cmd' -o -name '*~' \
		-o -name '.*.d' -o -name '.*.tmp' -o -name '*.mod.c' -o -name '.#*' -o -name 'cscope*' \) \
		-type f -print | xargs rm -f
	@rm -f *.map
	@rm -f *.dis

