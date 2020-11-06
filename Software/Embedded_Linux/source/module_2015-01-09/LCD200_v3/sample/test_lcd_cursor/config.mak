AS	=$(CROSS_COMPILE)as
LD	=$(CROSS_COMPILE)ld
CC	=$(CROSS_COMPILE)gcc
CPP	=$(CC) -E
CXX	=$(CROSS_COMPILE)g++
AR	=$(CROSS_COMPILE)ar
NM	=$(CROSS_COMPILE)nm
STRIP	=$(CROSS_COMPILE)strip
OBJCOPY =$(CROSS_COMPILE)objcopy
OBJDUMP =$(CROSS_COMPILE)objdump
RANLIB	=$(CROSS_COMPILE)RANLIB

# The command used to delete file.
RM        = rm -f

## Stable Section: usually no need to be changed. But you can add more.
##=============================================================================
SHELL   = /bin/sh
SOURCES = $(foreach d,$(SRCDIRS),$(wildcard $(addprefix $(d)/*,$(SRCEXTS))))
OBJS    = $(foreach x,$(SRCEXTS), \
      $(patsubst %$(x),%.o,$(filter %$(x),$(SOURCES))))
DEPS    = $(patsubst %.o,%.d,$(OBJS))

# Rules for creating the dependency files (.d).
#---------------------------------------------------
%.d : %.c
	@$(CC) -MM -MD $(CFLAGS) $<

%.d : %.C
	@$(CC) -MM -MD $(CXXFLAGS) $<

%.d : %.cc
	@$(CC) -MM -MD $(CXXFLAGS) $<

%.d : %.cpp
	@$(CC) -MM -MD $(CXXFLAGS) $<

%.d : %.CPP
	@$(CC) -MM -MD $(CXXFLAGS) $<

%.d : %.c++
	@$(CC) -MM -MD $(CXXFLAGS) $<

%.d : %.cp
	@$(CC) -MM -MD $(CXXFLAGS) $<

%.d : %.cxx
	@$(CC) -MM -MD $(CXXFLAGS) $<

# Rules for producing the objects.
#---------------------------------------------------
objs : $(OBJS)

%.o : %.c
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $<

%.o : %.C
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $<

%.o : %.cc
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $<

%.o : %.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $<

%.o : %.CPP
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $<

%.o : %.c++
	$(CXX -c $(CPPFLAGS) $(CXXFLAGS) $<

%.o : %.cp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $<

%.o : %.cxx
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $<
