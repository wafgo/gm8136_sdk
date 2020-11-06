#############################################################
#
# Boost
#
#############################################################

BOOST_VERSION = 1.47.0
BOOST_FILE_VERSION = $(subst .,_,$(BOOST_VERSION))
BOOST_SOURCE = boost_$(BOOST_FILE_VERSION).tar.bz2
BOOST_SITE = http://$(BR2_SOURCEFORGE_MIRROR).dl.sourceforge.net/sourceforge/boost
BOOST_INSTALL_STAGING = YES

TARGET_CC_VERSION = $(shell $(TARGET_CC) --version | head -n 1 | sed -e "s/.*[[:space:]]\([[:digit:].]\+$$\)/\1/g" )

BOOST_DEPENDENCIES = bzip2 zlib

BOOST_FLAGS =
BOOST_WITHOUT_FLAGS = python

BOOST_WITHOUT_FLAGS += $(if $(BR2_PACKAGE_BOOST_CHRONO),,chrono)
BOOST_WITHOUT_FLAGS += $(if $(BR2_PACKAGE_BOOST_DATE_TIME),,date_time)
BOOST_WITHOUT_FLAGS += $(if $(BR2_PACKAGE_BOOST_EXCEPTION),,exception)
BOOST_WITHOUT_FLAGS += $(if $(BR2_PACKAGE_BOOST_FILESYSTEM),,filesystem)
BOOST_WITHOUT_FLAGS += $(if $(BR2_PACKAGE_BOOST_GRAPH),,graph)
BOOST_WITHOUT_FLAGS += $(if $(BR2_PACKAGE_BOOST_GRAPH_PARALLEL),,graph_parallel)
BOOST_WITHOUT_FLAGS += $(if $(BR2_PACKAGE_BOOST_IOSTREAMS),,iostreams)
BOOST_WITHOUT_FLAGS += $(if $(BR2_PACKAGE_BOOST_MATH),,math)
BOOST_WITHOUT_FLAGS += $(if $(BR2_PACKAGE_BOOST_MPI),,mpi)
BOOST_WITHOUT_FLAGS += $(if $(BR2_PACKAGE_BOOST_PROGRAM_OPTIONS),,program_options)
BOOST_WITHOUT_FLAGS += $(if $(BR2_PACKAGE_BOOST_RANDOM),,random)
BOOST_WITHOUT_FLAGS += $(if $(BR2_PACKAGE_BOOST_REGEX),,regex)
BOOST_WITHOUT_FLAGS += $(if $(BR2_PACKAGE_BOOST_SERIALIZATION),,serialization)
BOOST_WITHOUT_FLAGS += $(if $(BR2_PACKAGE_BOOST_SIGNALS),,signals)
BOOST_WITHOUT_FLAGS += $(if $(BR2_PACKAGE_BOOST_SYSTEM),,system)
BOOST_WITHOUT_FLAGS += $(if $(BR2_PACKAGE_BOOST_TEST),,test)
BOOST_WITHOUT_FLAGS += $(if $(BR2_PACKAGE_BOOST_THREAD),,thread)
BOOST_WITHOUT_FLAGS += $(if $(BR2_PACKAGE_BOOST_WAVE),,wave)

ifeq ($(BR2_PACKAGE_BOOST_ICU),y)
BOOST_FLAGS += --with-icu
BOOST_DEPENDENCIES += icu
else
BOOST_FLAGS += --without-icu
endif

BOOST_LINK = $(if $(BR2_PREFER_STATIC_LIB),static,shared)
BOOST_MULTI = $(if $(BR2_PACKAGE_BOOST_MULTITHREADED),multi,single)
BOOST_VARIANT = $(if $(BR2_ENABLE_DEBUG),debug,release)

BOOST_WITHOUT_FLAGS_COMMASEPERATED += $(subst $(space),$(comma),$(strip $(BOOST_WITHOUT_FLAGS)))
BOOST_FLAGS += $(if $(BOOST_WITHOUT_FLAGS_COMMASEPERATED), --without-libraries=$(BOOST_WITHOUT_FLAGS_COMMASEPERATED))

define BOOST_CONFIGURE_CMDS
	(cd $(@D) && ./bootstrap.sh $(BOOST_FLAGS))
	echo "using gcc : $(TARGET_CC_VERSION) : $(TARGET_CXX) : <cxxflags>\"$(TARGET_CXXFLAGS)\" <linkflags>\"$(TARGET_LDFLAGS)\" ;" > $(@D)/user-config.jam
	echo "" >> $(@D)/user-config.jam
endef

define BOOST_INSTALL_TARGET_CMDS
	(cd $(@D) && ./b2 -q -d+2 \
	--user-config=$(@D)/user-config.jam \
	toolset=gcc \
	variant=$(BOOST_VARIANT) \
	link=$(BOOST_LINK) \
	threading=$(BOOST_MULTI) \
	runtime-link=$(BOOST_LINK) \
	--prefix=$(TARGET_DIR)/usr \
	--layout=system install )
endef

define BOOST_INSTALL_STAGING_CMDS
	(cd $(@D) && ./bjam -d+2 \
	--user-config=$(@D)/user-config.jam \
	toolset=gcc \
	variant=$(BOOST_VARIANT) \
	link=$(BOOST_LINK) \
	threading=$(BOOST_MULTI) \
	runtime-link=$(BOOST_LINK) \
	--prefix=$(STAGING_DIR)/usr \
	--layout=system install)
endef

$(eval $(call GENTARGETS))
