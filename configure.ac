AC_INIT([synaesthesia], [2.4])
AC_CONFIG_SRCDIR([main.cc])
AM_INIT_AUTOMAKE

dnl Check for Emscripten
AS_CASE([$CC],
        [*emcc], [enable_emscripten="yes"],
        [enable_emscripten="no"])

AS_IF([test "x$enable_emscripten" = "xyes"],
      AC_MSG_NOTICE([auto-detected use of Emscripten]))

AC_ARG_ENABLE(
  [emscripten],
  [AC_HELP_STRING([--enable-emscripten=@<:@no/yes@:>@],
                  [compile to JavaScript using Emscripten])])

dnl AC_CANONICAL_HOST fails to detect Emscripten.
dnl This contains kludges to make things work.
AS_IF([test "x$enable_emscripten" = "xyes"],
      [host_alias=none
       ac_cv_exeext=.html
       with_x=no])

AM_CONDITIONAL([COMPILE_EMSCRIPTEN], [test "x$enable_emscripten" = "xyes"])

dnl Detect the canonical host system
AC_CANONICAL_HOST
windows_host="no"
unix_sound="no"
cd_player="no"
case $host_os in
  mingw*)
    windows_host="yes"
    cd_player="yes"
    LIBS="$LIBS -lshlwapi"
    ;;
  linux* | freebsd*)
    unix_sound="yes"
    cd_player="yes"
    ;;
esac

AM_CONFIG_HEADER(config.h)

dnl Emscripten applies unsafe optimizations with -O3, and could break things.
AS_IF([test "x$enable_emscripten" = "xyes"],
      [default_cflags="-O2 -s FORCE_ALIGNED_MEMORY=1"],
      [default_cflags="-O3"])
: ${CXXFLAGS="$default_cflags"}
: ${CFLAGS="$default_cflags"}

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

dnl Enable CD player interface if driver is available
AS_IF([test "x$cd_player" = "xyes"],
      [AC_DEFINE([HAVE_CD_PLAYER], [1],
                 [Define to build CD player interface])])

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
AC_CHECK_HEADERS([vga.h vgamouse.h])
AS_IF([test "x$ac_cv_header_vga_h" = "xyes" -a dnl
            "x$ac_cv_header_vgamouse_h" = "xyes" ],
      [AC_CHECK_LIB(vga,vga_init)])
AM_CONDITIONAL([COMPILE_SVGALIB], [test "x$ac_cv_lib_vga_vga_init" = "xyes"])

dnl Check for SDL
SDL_VERSION=1.2.0
AH_TEMPLATE([HAVE_SDL], [Set to 1 if SDL is available.])
dnl Emscripten sdl-config does nothing, and running the another sdl-config
dnl instead causes problems. With Emscripten, just assume SDL exists.
AS_IF([test "x$enable_emscripten" = "xyes"],
      [enable_sdl=1],
      [AM_PATH_SDL($SDL_VERSION, [enable_sdl=1], [enable_sdl=0])])
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
