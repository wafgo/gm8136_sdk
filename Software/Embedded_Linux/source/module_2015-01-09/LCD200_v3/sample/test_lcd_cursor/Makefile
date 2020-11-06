###############################################################################
#
# Generic Makefile for C/C++ Program
#
# uthor: whyglinux (whyglinux AT hotmail DOT com)
# Date:   2006/03/04

# Description:
# The makefile searches in <SRCDIRS> directories for the source files
# with extensions specified in <SOURCE_EXT>, then compiles the sources
# and finally produces the <PROGRAM>, the executable file, by linking
# the objectives.

# Usage:
#   $ make           compile and link the program.
#   $ make objs      compile only (no linking. Rarely used).
#   $ make clean     clean the objectives and dependencies.
#   $ make cleanall  clean the objectives, dependencies and executable.
#   $ make rebuild   rebuild the program. The same as make clean && make all.
#==============================================================================

## Customizing Section: adjust the following if necessary.
##=============================================================================

#The prefix for cross-compiler.
CROSS_COMPILE := /usr/src/arm-linux-3.3/toolchain_gnueabi-4.4.0_ARMv5TE/usr/bin/arm-unknown-linux-uclibcgnueabi-
#CROSS_COMPILE := arm-linux-
# The executable file name.
# It must be specified.
PROGRAM   := test_lcd_cur

# The directories in which source files reside.
# At least one path should be specified.
SRCDIRS   := .

# The source file types (headers excluded).
# At least one type should be specified.
# The valid suffixes are among of .c, .C, .cc, .cpp, .CPP, .c++, .cp, or .cxx.
# SRCEXTS   := .c      # C program
# SRCEXTS   := .cpp    # C++ program
# SRCEXTS   := .c .cpp # C/C++ program
SRCEXTS   := .c

#SDK_SRC=$(shell pwd | sed 's/\(.*\)\/private.*/\1/')/arm-linux-2.6
#FFMPEG_SRC=$(shell pwd | sed 's/\(.*\)\/private.*/\1/')/ffmpeg-0.4.8

SDK_SRC= /usr/src/arm-linux-3.3/

# The flags used by the cpp (man cpp for more).
CPPFLAGS  := -Wall -Werror 
CPPFLAGS  += 

# The compiling flags used only for C.
# If it is a C++ program, no need to set these flags.
# If it is a C and C++ merging program, set these flags for the C parts.
CFLAGS    :=
CFLAGS    += -I$(SDK_SRC)/module/include/lcd200_v3

# The compiling flags used only for C++.
# If it is a C program, no need to set these flags.
# If it is a C and C++ merging program, set these flags for the C++ parts.
CXXFLAGS  += -I$(SDK_SRC)/module/include/lcd200_v3

# The library and the link options ( C and C++ common).
LDFLAGS   := -lpthread 

#CFLAGS  = $(CXXFLAGS)

include Makefile.rules


.PHONY : all objs clean cleanall rebuild
all : $(PROGRAM)

include config.mak

# Rules for producing the executable.
#----------------------------------------------
$(PROGRAM) : $(OBJS)
ifeq ($(strip $(SRCEXTS)), .c)  # C file
	$(CC) -o $@ $^ $(LDFLAGS)
else                            # C++ file
	$(CXX) -o $@ $^ $(LDFLAGS)
endif
	@$(NM) -n $@ | grep -v '\( [aUw] \)\|\(__crc_\)\|\( \$[adt]\)' > $@.map
	@$(OBJDUMP) -d $@ > $@.dis
	@$(STRIP) $@


-include $(DEPS)

rebuild: clean all

clean :
	@$(RM) *.o *.d *.dis *.map
	@find . \( -name '*.[oas]' -o -name '.*.cmd' -o -name '*~' \
	-o -name '.*.d' -o -name '.*.tmp' -o -name '*.mod.c' -o -name '*.d' \) \
	-type f -print | xargs $(RM) -f


cleanall: clean
	@$(RM) $(PROGRAM) $(PROGRAM).exe

### End of the Makefile ##  Suggestions are welcome  ## All rights reserved ###
###############################################################################
