#!/bin/sh

# compile_cdrskin.sh  
# Copyright 2005 - 2013 Thomas Schmitt, scdbackup@gmx.net, GPL v2 or later
# to be executed within  ./libburn-*  resp ./cdrskin-*

debug_opts="-O2"
def_opts=
largefile_opts="-D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE=1"
fifo_opts=""
libvers="-DCdrskin_libburn_1_3_2"

# To be used if Makefile.am uses libburn_libburn_la_CFLAGS
# burn="libburn/libburn_libburn_la-"
burn="libburn/"

cleanup_src_or_obj="$burn"cleanup.o
libdax_msgs_o="$burn"libdax_msgs.o
libdax_audioxtr_o="$burn"libdax_audioxtr.o
do_strip=0
static_opts=
warn_opts="-Wall -Wextra -Wno-unused-parameter"
libcdio=
fifo_source="cdrskin/cdrfifo.c"
compile_cdrskin=1
compile_cdrfifo=0
compile_dewav=0

libcam=
os=$(uname -s)
case $os in
 *FreeBSD)
  libcam="-lcam"
esac

for i in "$@"
do
  if test "$i" = "-compile_cdrfifo"
  then
    compile_cdrfifo=1
  elif test "$i" = "-compile_dewav"
  then
    compile_dewav=1
  elif test "$i" = "-libburn_1_3_2"
  then
    libvers="-DCdrskin_libburn_1_3_2"
    libdax_audioxtr_o="$burn"libdax_audioxtr.o
    libdax_msgs_o="$burn"libdax_msgs.o
    cleanup_src_or_obj="$burn"cleanup.o
  elif test "$i" = "-libburn_svn"
  then
    libvers="-DCdrskin_libburn_1_3_3"
    libdax_audioxtr_o="$burn"libdax_audioxtr.o
    libdax_msgs_o="$burn"libdax_msgs.o
    cleanup_src_or_obj="$burn"cleanup.o
  elif test "$i" = "-newapi" -o "$i" = "-experimental"
  then
    def_opts="$def_opts -DCdrskin_new_api_tesT"
  elif test "$i" = "-no_largefile"
  then
    largefile_opts=	
  elif test "$i" = "-dvd_obs_64k"
  then
    def_opts="$def_opts -DCdrskin_dvd_obs_default_64K"
  elif test "$i" = "-do_not_compile_cdrskin"
  then
    compile_cdrskin=0
  elif test "$i" = "-do_diet"
  then
    fifo_source=
    def_opts="$def_opts -DCdrskin_extra_leaN"
    warn_opts=
  elif test "$i" = "-do_strip"
  then
    do_strip=1
  elif test "$i" = "-use_libburn_fifo"
  then
    fifo_opts="-DCdrskin_use_libburn_fifO"
  elif test "$i" = "-use_no_libburn_fifo"
  then
    fifo_opts=""
  elif test "$i" = "-use_no_cdrfifo"
  then
    fifo_source=
    fifo_opts="-DCdrskin_use_libburn_fifO -DCdrskin_no_cdrfifO"
  elif test "$i" = "-use_libburn_cleanup"
  then
    fifo_source=
    fifo_opts="-DCdrskin_use_libburn_cleanuP -DCdrskin_use_libburn_fifO -DCdrskin_no_cdrfifO"
  elif test "$i" = "-use_libcdio"
  then
    libcdio="-lcdio"
  elif test "$i" = "-g"
  then
    debug_opts="-g"
  elif test "$i" = "-help" -o "$i" = "--help" -o "$i" = "-h"
  then
    echo "cdrskin/compile_cdrskin.sh : to be executed within top level directory"
    echo "Options:"
    echo "  -compile_cdrfifo  compile program cdrskin/cdrfifo."
    echo "  -compile_dewav    compile program test/dewav without libburn."
    echo "  -libburn_1_3_2    set macro to match libburn-1.3.2"
    echo "  -libburn_svn      set macro to match current libburn-SVN."
    echo "  -dvd_obs_64k      64 KB default size for DVD/BD writing."
    echo "  -use_libcdio      link with -lcdio because libburn uses it."
    echo "  -do_not_compile_cdrskin  omit compilation of cdrskin/cdrskin."
    echo "  -use_no_libburn_fifo  use cdrfifo even for single track non-CD"
    echo "  -use_no_cdrfifo   always use fifo of libburn and never cdrfifo"
    echo "  -experimental     use newly introduced libburn features."
    echo "  -do_diet          produce capability reduced lean version."
    echo "  -do_strip         apply program strip to compiled programs."
    echo "  -g                produce debuggable programm."
    echo "  -static           compile with cc option -static."
    exit 0
  elif test "$i" = "-static"
  then
    static_opts="-static"
  fi
done


timestamp="$(date -u '+%Y.%m.%d.%H%M%S')"
echo "Version timestamp :  $(sed -e 's/#define Cdrskin_timestamP "//' -e 's/"$//' cdrskin/cdrskin_timestamp.h)"
echo "Build timestamp   :  $timestamp"

if test "$compile_cdrskin"
then
  echo "compiling program cdrskin/cdrskin.c $fifo_source $static_opts $debug_opts $libvers $fifo_opts $def_opts $cleanup_src_or_obj $libcdio $libcam"
  cc -I. \
    $warn_opts \
    $static_opts \
    $debug_opts \
    $libvers \
    $largefile_opts \
    $fifo_opts \
    $def_opts \
    \
    -DCdrskin_build_timestamP='"'"$timestamp"'"' \
    \
    -o cdrskin/cdrskin \
    \
    cdrskin/cdrskin.c \
    $fifo_source \
    \
    $cleanup_src_or_obj \
    \
    "$burn"async.o \
    "$burn"cdtext.o \
    "$burn"debug.o \
    "$burn"drive.o \
    "$burn"file.o \
    "$burn"init.o \
    "$burn"options.o \
    "$burn"source.o \
    "$burn"structure.o \
    \
    "$burn"sg.o \
    "$burn"write.o \
    "$burn"read.o \
    $libdax_audioxtr_o \
    $libdax_msgs_o \
    \
    "$burn"mmc.o \
    "$burn"sbc.o \
    "$burn"spc.o \
    "$burn"util.o \
    \
    "$burn"sector.o \
    "$burn"toc.o \
    \
    "$burn"crc.o \
    "$burn"ecma130ab.o \
    \
    $libcdio \
    $libcam \
    -lpthread

    ret=$?
    if test "$ret" = 0
    then
      dummy=dummy
    else
      echo >&2
      echo "+++ FATAL : Compilation of cdrskin failed" >&2
      echo >&2
      exit 1
    fi
fi

if test "$compile_cdrfifo" = 1
then
  echo "compiling program cdrskin/cdrfifo.c $static_opts $debug_opts"
  cc $static_opts $debug_opts \
     -DCdrfifo_standalonE \
     -o cdrskin/cdrfifo \
     cdrskin/cdrfifo.c

    ret=$?
    if test "$ret" = 0
    then
      dummy=dummy
    else
      echo >&2
      echo "+++ FATAL : Compilation of cdrfifo failed" >&2
      echo >&2
      exit 2
    fi
fi

if test "$compile_dewav" = 1
then
  echo "compiling program test/dewav.c -DDewav_without_libburN $static_opts $debug_opts"
  cc $static_opts $debug_opts \
     -DDewav_without_libburN \
     -o test/dewav \
     test/dewav.c \
     "$burn"libdax_audioxtr.o \
     "$burn"libdax_msgs.o \
     \
    -lpthread

    ret=$?
    if test "$ret" = 0
    then
      dummy=dummy
    else
      echo >&2
      echo "+++ FATAL : Compilation of test/dewav failed" >&2
      echo >&2
      exit 2
    fi
fi     

if test "$do_strip" = 1
then
  echo "stripping result cdrskin/cdrskin"
  strip cdrskin/cdrskin
  if test "$compile_cdrfifo" = 1
  then
    echo "stripping result cdrskin/cdrfifo"
    strip cdrskin/cdrfifo
  fi
fi

echo 'done.'
