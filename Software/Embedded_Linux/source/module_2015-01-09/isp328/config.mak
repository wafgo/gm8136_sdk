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

.PHONEY: ALL

ALL: modules

clobber: clean
	@find . \( -name '*.[oas]' -o -name '*.ko' -o -name '.*.cmd' -o -name '*~' \
		-o -name '.*.d' -o -name '.*.tmp' -o -name '*.mod.c' -o -name '.#*' -o -name 'cscope*' \
		-o -name '*.dis' -o -name '*.map' \) \
		-type f -print | xargs rm -f

cscope:
	@find ./ -name "*.c" -or -name "*.h" -or -name "*.cpp" -or -name "*.S" | cscope -Rbq
