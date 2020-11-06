AC_INIT([libburn], [1.3.2], [http://libburnia-project.org])
AC_PREREQ([2.50])
dnl AC_CONFIG_HEADER([config.h])	

AC_CANONICAL_HOST
AC_CANONICAL_TARGET

LIBBURNIA_SET_FLAGS

AM_INIT_AUTOMAKE([subdir-objects])
AC_CONFIG_MACRO_DIR([./])

dnl Notes about version numbers and .so numbers:
dnl
dnl Regrettably the meaning of the various version types was misunderstood
dnl before version 0.4.1.
dnl
dnl In the past MAJOR.MINOR.MICRO versions led to the following SONAME numbers:
dnl    0.2.2 = 2 , 0.2.3 = 3 , 0.2.6 = 6
dnl    0.3.0 = 0 , 0.3.2 = 2 , 0.3.4 = 4 . 0.3.6 = 6 , 0.3.8 = 4
dnl    0.4.0 = 0 (also released as SONAME 4)
dnl
dnl Meanwhile the following schemes are maintained in parallel:
dnl
dnl BURN_MAJOR_VERSION , BURN_MINOR_VERSION , BURN_MICRO_VERSION 
dnl    are three small non-negative integers which describe the evolution
dnl    steps of the library.
dnl    Older applications are able to use younger libraries over
dnl    quite a long range of such steps. Some day, nevertheless,
dnl    compatibility might get terminated, after due notice.
dnl
dnl SONAME (libburn.so.4)
dnl    is a small positive integer which marks a family of compatible
dnl    evolution steps. Libraries with a particular SONAME allow a binary
dnl    with the same SONAME to start up. Any further compatibility check is to
dnl    be done by own runtime means. Especially  *_version() calls in the API
dnl    which return BURN_MAJOR_VERSION, BURN_MINOR_VERSION, BURN_MICRO_VERSION.
dnl    See below.
dnl
dnl CURRENT, AGE, REVISION
dnl    are three integers used by libtool. CURRENT is positive, the others
dnl    non-negative. The use at runtime is not known yet. But libtool computes
dnl    at build time  SONAME = CURRENT - AGE.
dnl    So this is a superspace of the SONAME version space. To avoid
dnl    ill SONAME, the value of  CURRENT must be larger than AGE.
dnl    See also http://www.gnu.org/software/libtool/manual.html#Interfaces
dnl
dnl On Linux the name of the dynamic library will be
dnl    libburn.so.$SONAME.$AGE.$REV
dnl In the terminology of this file:
dnl    CURRENT = LT_CURRENT
dnl    AGE     = LT_AGE
dnl    REVISION= LT_REVISION
dnl
dnl Beginning with libburn-0.4.1 a rectified counting was introduced as
dnl    CURRENT=10, REVISION=1, AGE=6
dnl This rectification declared that version to be binary compatible up
dnl from libburn-0.3.4.
dnl Real compatibility was given since libburn-0.3.2.
dnl Beware of libburn-0.2.6 which had SONAME=6 and is not binary compatible.
dnl Applications for libburn-0.2 to libburn-0.3.1 need recompilation but no
dnl source code changes.
dnl
dnl Neatly versioned stable releases meanwhile:
dnl    0.4.2 = libburn.so.4.7.0
dnl    0.4.4 = libburn.so.4.9.0
dnl    0.4.6 = libburn.so.4.11.0
dnl    0.4.8 = libburn.so.4.13.0
dnl    0.5.0 = libburn.so.4.15.0
dnl    0.5.2 = libburn.so.4.17.0
dnl    0.5.4 = libburn.so.4.19.0
dnl    0.5.6 = libburn.so.4.21.0
dnl    0.5.8 = libburn.so.4.23.0
dnl    0.6.0 = libburn.so.4.25.0
dnl    0.6.2 = libburn.so.4.27.0
dnl    0.6.4 = libburn.so.4.29.0
dnl    0.6.6 = libburn.so.4.31.0
dnl    0.6.8 = libburn.so.4.33.0
dnl    0.7.0 = libburn.so.4.35.0
dnl    0.7.2 = libburn.so.4.37.0
dnl    0.7.4 = libburn.so.4.39.0
dnl    0.7.6 = libburn.so.4.41.0
dnl    0.7.8 = libburn.so.4.43.0
dnl    0.8.0 = libburn.so.4.45.0
dnl    0.8.2 = libburn.so.4.47.0
dnl    0.8.4 = libburn.so.4.49.0
dnl    0.8.6 = libburn.so.4.51.0
dnl    0.8.8 = libburn.so.4.53.0
dnl    0.9.0 = libburn.so.4.55.0
dnl    1.0.0 = libburn.so.4.57.0
dnl    1.0.2 = libburn.so.4.59.0
dnl    1.0.4 = libburn.so.4.61.0
dnl    1.0.6 = libburn.so.4.63.0
dnl    1.1.0 = libburn.so.4.65.0
dnl    1.1.4 = libburn.so.4.67.0
dnl    1.1.6 = libburn.so.4.69.0
dnl    1.1.8 = libburn.so.4.71.0
dnl    1.2.0 = libburn.so.4.73.0
dnl    1.2.2 = libburn.so.4.75.0
dnl    1.2.4 = libburn.so.4.77.0
dnl    1.2.6 = libburn.so.4.79.0
dnl    1.2.8 = libburn.so.4.81.0
dnl    1.3.0 = libburn.so.4.83.0
dnl    1.3.2 = libburn.so.4.85.0
dnl
dnl So LT_CURRENT, LT_REVISION and LT_AGE get set directly here.
dnl SONAME of the emerging library is  LT_CURRENT - LT_AGE.
dnl The linker will do no finer checks. Especially no age range check for
dnl the application binary. If SONAME matches, then the couple starts.
dnl 
dnl Therefore at run time info is provided by libburn function burn_version().
dnl It returns the major, minor and micro revision of the library.
dnl Before using any API feature, a program should check for age.
dnl
dnl The variables BURN_*_VERSION are mere copies for informing libtool.
dnl The true values which get issued and should be compared are macros
dnl defined in libburn/libburn.h .
dnl
dnl Normally one can allow a program to run with a library which passed the
dnl linker SONAME test and which is not older than the library it was
dnl developed for. Library2 is younger than library1 if:
dnl   major2>major1 || (major2==major1 &&
dnl                     (minor2>minor1 || (minor2==minor1 && micro2 > micro1)))
dnl
dnl If BURN_*_VERSION changes, be sure to change AC_INIT above to match.
dnl
dnl As said: Only copies. Original in libburn/libburn.h : burn_header_version_*
BURN_MAJOR_VERSION=1
BURN_MINOR_VERSION=3
BURN_MICRO_VERSION=2
BURN_VERSION=$BURN_MAJOR_VERSION.$BURN_MINOR_VERSION.$BURN_MICRO_VERSION

AC_SUBST(BURN_MAJOR_VERSION)
AC_SUBST(BURN_MINOR_VERSION)
AC_SUBST(BURN_MICRO_VERSION)
AC_SUBST(BURN_VERSION)

dnl Libtool versioning
LT_RELEASE=$BURN_MAJOR_VERSION.$BURN_MINOR_VERSION.$BURN_MICRO_VERSION
dnl
dnl This is the release version libburn-1.3.2
dnl ### This is the development version after above release version
dnl LT_CURRENT++, LT_AGE++ has not yet happened.
dnl ### LT_CURRENT++, LT_AGE++ has happened meanwhile.
dnl
dnl SONAME = 89 - 85 = 4 . Linux library name = libburn.so.4.85.0
LT_CURRENT=89
LT_AGE=85
LT_REVISION=0
LT_CURRENT_MINUS_AGE=`expr $LT_CURRENT - $LT_AGE`

AC_SUBST(LT_RELEASE)
AC_SUBST(LT_CURRENT)
AC_SUBST(LT_REVISION)
AC_SUBST(LT_AGE)
AC_SUBST(LT_CURRENT_MINUS_AGE)

dnl ts A71207: This is done only not to break any old components
BURN_INTERFACE_AGE=$LT_REVISION
BURN_BINARY_AGE=`expr $LT_AGE + $BURN_INTERFACE_AGE`
AC_SUBST(BURN_INTERFACE_AGE)
AC_SUBST(BURN_BINARY_AGE)

AC_PREFIX_DEFAULT([/usr/local])
test "$prefix" = "NONE" && prefix=$ac_default_prefix

AM_MAINTAINER_MODE

AM_PROG_CC_C_O
AC_C_CONST
AC_C_INLINE
AC_C_BIGENDIAN

dnl Large file support
AC_SYS_LARGEFILE
AC_FUNC_FSEEKO
AC_CHECK_FUNC([fseeko])
if test ! $ac_cv_func_fseeko; then
   AC_ERROR([Libburn requires largefile support.])
fi

AC_PROG_LIBTOOL
AC_SUBST(LIBTOOL_DEPS)
LIBTOOL="$LIBTOOL --silent"

AC_PROG_INSTALL

AC_CHECK_HEADERS()

THREAD_LIBS=-lpthread
AC_SUBST(THREAD_LIBS)

TARGET_SHIZZLE
AC_SUBST(ARCH)
AC_SUBST(LIBBURNIA_PKGCONFDIR)
AC_SUBST(LIBBURN_ARCH_LIBS)

dnl ts A90303
dnl Check the preconditions for using statvfs() in sg-dummy
dnl (sg-linux and sg-freebsd use statvfs() unconditionally)
STATVFS_DEF=-DLibburn_os_has_statvfS
AC_CHECK_HEADER(sys/statvfs.h, X=, STATVFS_DEF=)
AC_CHECK_FUNC([statvfs], X=, STATVFS_DEF=)
dnl If this would be done more specifically in Makefile.am
dnl via libburn_libburn_la_CFLAGS then undesired .o file names would emerge
CFLAGS="$STATVFS_DEF $CFLAGS"

dnl ts A91122
AC_ARG_ENABLE(track-src-odirect,
[  --enable-track-src-odirect Enable use of O_DIRECT with track input, default=no],
               , enable_track_src_odirect=no)
if test x$enable_track_src_odirect = xyes; then
    LIBBURN_O_DIRECT_DEF="-DLibburn_read_o_direcT"
    echo "enabled  use of O_DIRECT with track input"
else
    LIBBURN_O_DIRECT_DEF=
    echo "disabled use of O_DIRECT with track input"
fi
dnl Avoid the need for libburn_libburn_la_CFLAGS in Makefile.am (ugly .o names)
dnl ### AC_SUBST(LIBBURN_O_DIRECT_DEF)
CFLAGS="$LIBBURN_O_DIRECT_DEF $CFLAGS"

dnl ts A91116
AC_ARG_ENABLE(dvd-obs-64k,
[  --enable-dvd-obs-64k    64 KB default size for DVD writing, default=no],
               , enable_dvd_obs_64k=no)
if test x$enable_dvd_obs_64k = xyes; then
    LIBBURN_DVD_OBS_64K="-DLibburn_dvd_obs_default_64K"
    echo "enabled  write size default 64 KB on DVD"
else
    LIBBURN_DVD_OBS_64K=
    echo "disabled write size default 64 KB on DVD"
fi
CFLAGS="$LIBBURN_DVD_OBS_64K $CFLAGS"

dnl ts B20413
AC_ARG_ENABLE(dvd-obs-pad,
[  --enable-dvd-obs-pad    pad DVD DAO sessions to 32 resp. 64 KB, default=no],
               , enable_dvd_obs_pad=no)
if test x$enable_dvd_obs_pad = xyes; then
    LIBBURN_DVD_OBS_PAD="-DLibburn_dvd_always_obs_paD"
    echo "enabled  padding of DVD DAO sessions to 32 resp. 64 KB"
else
    LIBBURN_DVD_OBS_64K=
    echo "disabled padding of DVD DAO sessions to 32 resp. 64 KB"
fi
CFLAGS="$LIBBURN_DVD_OBS_PAD $CFLAGS"

dnl ts A91218 - B21002
case $host_os in
    cygwin*|mingw*)
        default_libcdio=yes
        ;;
    *)
        default_libcdio=no
        ;;
esac
# Check for proper library versions if this is desired.
# (It fails too often on too many systems.)
AC_ARG_ENABLE(pkg-check-modules,
[  --enable-pkg-check-modules      Enable pkg-config check for libcdio , default=no],
               , enable_pkg_check_modules=no)
AC_ARG_ENABLE(libcdio,
[  --enable-libcdio        Enable use of libcdio as system adapter, default=no (except on MSWindows)],
               , enable_libcdio=$default_libcdio)
PKG_PROG_PKG_CONFIG
if test x$enable_libcdio = xyes; then
dnl Check whether there is libcdio-devel and libcdio-runtime.
dnl If not, erase this macro
    LIBCDIO_DEF="-DLibburn_use_libcdiO" 
dnl The empty yes case obviously causes -lcdio to be linked
    AC_CHECK_HEADER(cdio/cdio.h, AC_CHECK_LIB(cdio, mmc_last_cmd_sense, , LIBCDIO_DEF= ), LIBCDIO_DEF= )
else
    LIBCDIO_DEF=
fi
if test x$LIBCDIO_DEF = x
then
    if test x$enable_libcdio = xyes
    then
      echo "WARNING: could not enable use of libcdio as system adapter"
    fi
else
    echo "enabled  use of libcdio as system adapter"
    CFLAGS="$LIBCDIO_DEF $CFLAGS"

    if test x$enable_pkg_check_modules = xyes; then
        LIBCDIO_REQUIRED=0.83
        PKG_CHECK_MODULES(LIBCDIO, libcdio >= $LIBCDIO_REQUIRED)
    else
        echo "checking for LIBCDIO... skipped, no --enable-pkg-check-modules"
    fi
fi

dnl ts B00704
# Library versioning normally serves a complex purpose.
# Since libburn obeys strict ABI backward compatibility, it needs only the
# simple feature to declare function names "global:" or "local:". Only the
# global ones are visible to applications at library load time.
AC_ARG_ENABLE(versioned-libs,
[  --enable-versioned-libs Enable strict symbol encapsulation , default=yes],
               , enable_versioned_libs=yes)
if test x$enable_versioned_libs = xyes; then
    vers_libs_test=no
    LIBBURN_ASSERT_VERS_LIBS
    if test x$vers_libs_test = xno
    then 
        echo "disabled strict symbol encapsulation (test failed)"
    else
        echo "enabled  strict symbol encapsulation"
    fi
else
    echo "disabled strict symbol encapsulation"
fi

AC_ARG_ENABLE(ldconfig-at-install,
[  --enable-ldconfig-at-install    On GNU/Linux run ldconfig, default=yes],
               , ldconfig_at_install=yes)
if test x$ldconfig_at_install = xyes; then 
    dummy=dummy
else
    LIBBURNIA_LDCONFIG_CMD="echo 'NOTE: ldconfig is disabled. If needed, configure manually for:'"
    echo "disabled run of ldconfig during installation on GNU/Linux"
fi  
AC_SUBST(LIBBURNIA_LDCONFIG_CMD)


dnl Add compiler-specific flags

dnl See if the user wants aggressive optimizations of the code
AC_ARG_ENABLE(debug,
[  --enable-debug          Disable aggressive optimizations [default=yes]],
              , enable_debug=yes)
if test x$enable_debug != xyes; then
    if test x$GCC = xyes; then
        CFLAGS="-O3 $CFLAGS"
        CFLAGS="-fexpensive-optimizations $CFLAGS"
    fi
    CFLAGS="-DNDEBUG $CFLAGS"
else
    if test x$GCC = xyes; then
        CFLAGS="-g -pedantic -Wall -Wextra -Wno-unused-parameter $CFLAGS"
    fi
    CFLAGS="-DDEBUG $CFLAGS"
fi

dnl Determine target directory for libburn-*.pc
dnl Important: Must be performed _after_ TARGET_SHIZZLE
dnl
LIBBURNIA_SET_PKGCONFIG


AC_CONFIG_FILES([
	Makefile
	doc/doxygen.conf
	version.h
	libburn-1.pc
	])
AC_OUTPUT
