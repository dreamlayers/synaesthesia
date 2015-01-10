AC_INIT([synaesthesia], [2.4])
AC_CONFIG_SRCDIR([main.cc])
AM_INIT_AUTOMAKE

dnl Check for Emscripten
AS_CASE([$CXX],
        [*em++], [enable_emscripten="yes"],
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

AC_ARG_ENABLE(
  [winamp],
  [AC_HELP_STRING([--enable-winamp=@<:@no/yes@:>@],
                  [compile Winamp visualization plugin])])

dnl Detect the canonical host system
AC_CANONICAL_HOST
windows_host="no"
windows_sound="no"
winamp_host="no"
unix_sound="no"
cd_player="no"
AS_CASE([$host_os],
  [mingw*],
    [AS_IF([test "x$enable_winamp" = "xyes"],
           [windows_sound="no"],
           [windows_sound="yes"])
     windows_host="yes"
     cd_player="yes"
     LIBS="$LIBS -lshlwapi -lwinmm"],
  [linux* | freebsd*],
    [unix_sound="yes"
     cd_player="yes"]
)

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

AS_IF([test "x$enable_winamp" = "xyes"],
      [AC_DEFINE([WINAMP], [1],
                 [Define to build Winamp plugin])])

dnl Test endian. This works even if cross compiling.
AC_C_BIGENDIAN([AC_DEFINE([BIGENDIAN], [], [Host is big endian.])],
               [AC_DEFINE([LITTLEENDIAN], [], [Host is litle endian.])])

dnl Test for getpwuid(), getuid() and pwd.h
AC_CHECK_HEADERS([pwd.h])
AC_CHECK_FUNCS([getpwuid getuid])

dnl Operating system specific Automake conditionals
AM_CONDITIONAL([COMPILE_WINDOWS], [test "x$windows_sound" = "xyes"])
AM_CONDITIONAL([COMPILE_WINAMP], [test "x$enable_winamp" = "xyes"])
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

dnl Check for esound
AC_CHECK_LIB(esd,esd_record_stream_fallback)

dnl Check for SDL
AC_ARG_WITH(sdl2, [AC_HELP_STRING([--with-sdl2=@<:@no/yes/path@:>@],
                  [Build using SDL 2.0 instead of SDL 1.2.
                   If using Emscripten, provide path to SDL-emscripten.])],
                   [], [with_sdl2=no])
AH_TEMPLATE([HAVE_SDL], [Set to 1 if SDL is available.])
dnl Emscripten sdl-config does nothing, and running the another sdl-config
dnl instead causes problems. Assume SDL 1.2 exists as part of Emscripten.
dnl For SDL 2, a path needs to be specified.
AS_IF([test "x$enable_emscripten" = "xyes"],
      [enable_sdl=1
       AS_IF([test "x$with_sdl2" != "xno"],
             [CXXFLAGS="$CFLAGS -Wno-warn-absolute-paths -I$with_sdl2/include"
              CXXFLAGS="$CXXFLAGS -Wno-warn-absolute-paths -I$with_sdl2/include"
              LIBS="$LIBS -L$with_sdl2/build/.libs/ -lSDL2"])],
      [AS_IF([test "x$with_sdl2" = "xno"],
             [AM_PATH_SDL([1.2.0], [enable_sdl=1], [enable_sdl=0])],
             [AM_PATH_SDL2([2.0.0], [enable_sdl=1], [enable_sdl=0])])
       AS_IF([test "x$enable_sdl" != "x0"],
             [CFLAGS="$CFLAGS $SDL_CFLAGS"
              CXXFLAGS="$CXXFLAGS $SDL_CFLAGS"
              LIBS="$LIBS $SDL_LIBS"])])
AS_IF([test "x$enable_sdl" != "x0"],
      [AC_DEFINE([HAVE_SDL], [1])],
      [AC_DEFINE([HAVE_SDL], [0])])

AM_CONDITIONAL([COMPILE_SDL], [test "x$enable_sdl" != "x0"])

AC_OUTPUT(Makefile)
