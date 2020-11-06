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
INDENT = indent

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

debug_info: $(patsubst %.o,%.map,$(obj-m)) $(patsubst %.o,%.dis,$(obj-m))

clobber: clean
	@find . \( -name '*.[oas]' -o -name '*.ko' -o -name '.*.cmd' -o -name '*~' \
		-o -name '.*.d' -o -name '.*.tmp' -o -name '*.mod.c' -o -name '.#*' \) \
		-type f -print | xargs rm -f
	@find ./ -name "*.map" | xargs rm -f
	@./ -name "*.dis" | xargs rm -f

indent:
	@find ./ -name "*.c" -or -name "*.h" |xargs $(INDENT) -npro -kr -nut -i4 -sob -l100 -ss -ncs -cp1
	@find ./ -name "*.c~" -or -name "*.h~" |xargs  rm -f
	@find ./ -name "*.c" -or -name "*.h" -or -name "*.mak" -or -name "Makefile" \
		-or -name "Makefile.*" -or -name "Change*" | xargs chmod 644
