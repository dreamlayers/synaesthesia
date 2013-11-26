AC_INIT([synaesthesia], [2.4])
AC_CONFIG_SRCDIR([main.cc])

dnl Detect the canonical host system
AC_CANONICAL_HOST
windows_host="no"
unix_sound="no"
case $host_os in
  mingw*)
    windows_host="yes"
    ;;
  linux* | freebsd*)
    unix_sound="yes"
    ;;
esac

AM_INIT_AUTOMAKE

AM_CONFIG_HEADER(config.h)

CXXFLAGS="-O4"
CFLAGS="-O4"

AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL

dnl Test endian. This works even if cross compiling.
AC_C_BIGENDIAN([AC_DEFINE([BIGENDIAN], [], [Host is big endian.])],
               [AC_DEFINE([LITTLEENDIAN], [], [Host is litle endian.])])

dnl Test for getpwuid(), getuid() and pwd.h
AC_CHECK_HEADERS([pwd.h])
AC_CHECK_FUNCS([getpwuid getuid])

dnl Operating system specific Automake conditionals
AM_CONDITIONAL([COMPILE_WINDOWS], [test "x$windows_host" = "xyes"])
AM_CONDITIONAL([COMPILE_UNIXSOUND], [test "x$unix_sound" = "xyes"])

dnl Possibility to use PortAudio instead of native sound
AC_ARG_WITH([portaudio],
  [AS_HELP_STRING([--with-portaudio],
    [Use PortAudio instead of native sound])],
  [],
  [with_portaudio=no])

AS_IF([test "x$with_portaudio" != xno],
  [AC_CHECK_LIB([portaudio], [Pa_Initialize],
    [with_portaudio=yes
     LIBS="$LIBS -lportaudio"
     AC_DEFINE([HAVE_PORTAUDIO], [1],
               [Define to use PortAudio instead of native audio])
    ],
    [with_portaudio=no
     AC_MSG_FAILURE(
       [--with-portaudio was given, but test for PortAudio failed])
    ])
  ])
AM_CONDITIONAL([COMPILE_PORTAUDIO], [test "x$with_portaudio" = "xyes"])

dnl Use libwinmm for sound and CD control on Windows
if test "x$windows_host" = "xyes" ; then
LIBS="$LIBS -lwinmm"
fi

dnl Don't use X on Microsoft Windows. Cross-compiling from
dnl Cygwin coululd include /usr/include Cygwin headers.
if test "x$windows_host" = "xyes" ; then
  no_x=yes
  AC_DEFINE([X_DISPLAY_MISSING], [], [X not being used on Microsoft Windows])
else
  dnl Check for X
  AC_PATH_XTRA
  if test "x$no_x" != xyes ; then
    CFLAGS="$CFLAGS $X_CFLAGS"
    CXXFLAGS="$CXXFLAGS $X_CFLAGS"
    LIBS="$LIBS $X_PRE_LIBS $X_LIBS $X_EXTRA_LIBS -lX11"
  fi
fi
AM_CONDITIONAL([COMPILE_X11], [test "x$no_x" != "xyes"])

dnl Check for svgalib
AC_CHECK_LIB(vga,vga_init)
AM_CONDITIONAL([COMPILE_SVGALIB], [test "x$ac_cv_lib_vga_vga_init" = "xyes"])

dnl Check for SDL
SDL_VERSION=1.2.0
AH_TEMPLATE([HAVE_SDL], [Set to 1 if SDL is available.])
AM_PATH_SDL($SDL_VERSION, [enable_sdl=1], [enable_sdl=0])
if test "x$enable_sdl" != x0 ; then
AC_DEFINE([HAVE_SDL], [1])
CFLAGS="$CFLAGS $SDL_CFLAGS"
CXXFLAGS="$CXXFLAGS $SDL_CFLAGS"
LIBS="$LIBS $SDL_LIBS"
else
AC_DEFINE([HAVE_SDL], [0])
fi
AM_CONDITIONAL([COMPILE_SDL], [test "x$enable_sdl" != "x0"])

dnl Check for esound
AC_CHECK_LIB(esd,esd_record_stream_fallback)

AC_OUTPUT(Makefile)
