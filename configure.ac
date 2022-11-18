AC_INIT([synaesthesia],[2.4])
AC_CONFIG_SRCDIR([main.cc])
AM_INIT_AUTOMAKE

dnl Check for Emscripten
AS_CASE([$CXX],
        [*em++], [enable_emscripten="yes"],
        [enable_emscripten="no"])

AS_IF([test "x$enable_emscripten" = "xyes"],
      [AC_MSG_NOTICE([auto-detected use of Emscripten])])

AC_ARG_ENABLE(
  [emscripten],
  [AS_HELP_STRING([--enable-emscripten=@<:@no/yes@:>@],[compile to JavaScript using Emscripten])])

dnl AC_CANONICAL_HOST fails to detect Emscripten.
dnl This contains kludges to make things work.
AS_IF([test "x$enable_emscripten" = "xyes"],
      [host_alias=none
       ac_cv_exeext=.html])

AM_CONDITIONAL([COMPILE_EMSCRIPTEN], [test "x$enable_emscripten" = "xyes"])

AC_ARG_ENABLE(
  [winamp],
  [AS_HELP_STRING([--enable-winamp=@<:@no/yes@:>@],[compile Winamp visualization plugin])])

AC_ARG_ENABLE(
  [audacious],
  [AS_HELP_STRING([--enable-audacious=@<:@no/yes@:>@],[compile Audacious visualization plugin])])

dnl Detect the canonical host system
AC_CANONICAL_HOST
windows_host="no"
windows_sound="no"
winamp_host="no"
audacious_host="no"
unix_sound="no"
cd_player="no"
default_cflags="-O3"
AS_CASE([$host_os],
  [mingw*],
    [AS_IF([test "x$enable_winamp" = "xyes"],
           [windows_sound="no"],
           [windows_sound="yes"])
     windows_host="yes"
     cd_player="yes"
     LIBS="$LIBS -lshlwapi -lwinmm"],
  [linux* | freebsd*],
    [AS_IF([test "x$enable_audacious" = "xyes"],
           [unix_sound="no"
            cd_player="no"
            default_cflags="$default_cflags -fPIC"],
           [unix_sound="yes"
            cd_player="yes"])]
)

AC_CONFIG_HEADERS(config.h)

: ${CXXFLAGS="$default_cflags"}
: ${CFLAGS="$default_cflags"}

AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
PKG_PROG_PKG_CONFIG

AS_IF([test "x$enable_winamp" = "xyes"],
      [AC_DEFINE([WINAMP], [1],
                 [Define to build Winamp plugin])])

AS_IF([test "x$enable_audacious" = "xyes"],
      [AC_DEFINE([AUDACIOUS], [1],
                 [Define to build Audacious plugin])
       AX_PTHREAD()
       LIBS="$PTHREAD_LIBS $LIBS"
       CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
       CXXFLAGS="$CXXFLAGS $PTHREAD_CFLAGS"
       PKG_CHECK_MODULES([AUDACIOUS], [audacious glib-2.0 dbus-glib-1 dbus-1])])

dnl Test endian. This works even if cross compiling.
AC_C_BIGENDIAN([AC_DEFINE([BIGENDIAN], [], [Host is big endian.])],
               [AC_DEFINE([LITTLEENDIAN], [], [Host is litle endian.])])

dnl Test for getpwuid(), getuid() and pwd.h
AC_CHECK_HEADERS([pwd.h])
AC_CHECK_FUNCS([getpwuid getuid])

dnl Operating system specific Automake conditionals
AM_CONDITIONAL([COMPILE_WINDOWS], [test "x$windows_host" = "xyes"])
AM_CONDITIONAL([COMPILE_WINSOUND], [test "x$windows_sound" = "xyes"])
AM_CONDITIONAL([COMPILE_WINAMP], [test "x$enable_winamp" = "xyes"])
AM_CONDITIONAL([COMPILE_AUDACIOUS], [test "x$enable_audacious" = "xyes"])
AM_CONDITIONAL([COMPILE_UNIXSOUND], [test "x$unix_sound" = "xyes"])

dnl Enable CD player interface if driver is available
AS_IF([test "x$cd_player" = "xyes"],
      [AC_DEFINE([HAVE_CD_PLAYER], [1],
                 [Define to build CD player interface])])

dnl Possibility to use PortAudio instead of native sound
AH_TEMPLATE([HAVE_PORTAUDIO], [Set to 1 if PortAudio is used.])
AC_ARG_WITH([portaudio],
  [AS_HELP_STRING([--with-portaudio],
    [Use PortAudio instead of native sound])],
  [],
  [with_portaudio=no])

AS_IF([test "x$with_portaudio" != "xno"],
  dnl Try to statically link PortAudio in Windows
  [AS_IF([test "x$windows_host" = "xyes"],
         [PKG_CHECK_MODULES_STATIC([PortAudio], [portaudio-2.0],
                                   [enable_portaudio=1], [enable_portaudio=0])],
         [enable_portaudio=0])
   AS_IF([test "x$enable_portaudio" != "x1"],
         [PKG_CHECK_MODULES([PortAudio], [portaudio-2.0],
                            [enable_portaudio=1],
                            [AC_MSG_FAILURE([--with-portaudio was given, but test for PortAudio failed])])])
   CFLAGS="$CFLAGS $PortAudio_CFLAGS"
   CXXFLAGS="$CXXFLAGS $PortAudio_CFLAGS"
   LIBS="$LIBS $PortAudio_LIBS"
   AC_DEFINE([HAVE_PORTAUDIO], [1])])
AM_CONDITIONAL([COMPILE_PORTAUDIO], [test "x$with_portaudio" = "xyes"])

dnl Check for esound
AC_CHECK_LIB(esd,esd_record_stream_fallback)

dnl Check for SDL
AC_ARG_WITH(sdl2, [AS_HELP_STRING([--with-sdl2=@<:@no/yes@:>@],[Build using SDL 2 instead of SDL 1.2.])],
                  [], [with_sdl2=no])
dnl pkg-config SDL 2 detection would not work with Emscripten
AS_IF([test "x$enable_emscripten" = "xyes"],
      [AS_IF([test "x$with_sdl2" = "xyes"],
             [CFLAGS="$CFLAGS -s USE_SDL=2"
              CXXFLAGS="$CXXFLAGS -s USE_SDL=2"])],
      dnl On Windows, first try to statically link SDL
      [enable_sdl=0
       AS_IF([test "x$windows_host" = "xyes"],
             [AS_IF([test "x$with_sdl2" = "xno"],
                    [PKG_CHECK_MODULES_STATIC([SDL], [sdl], [enable_sdl=1], [enable_sdl=0])],
                    [PKG_CHECK_MODULES_STATIC([SDL], [sdl2], [enable_sdl=1], [enable_sdl=0])])])
       AS_IF([test "x$enable_sdl" != "x1"],
             [AS_IF([test "x$with_sdl2" = "xno"],
                    [PKG_CHECK_MODULES([SDL], [sdl], [enable_sdl=1], [enable_sdl=0])],
                    [PKG_CHECK_MODULES([SDL], [sdl2], [enable_sdl=1], [enable_sdl=0])])])
       dnl Windows SDL does not ship with pkg-config .pc file.
       dnl Builds can still fail when static linking in Windows.
       dnl AM_PATH_SDL uses obsolete macros, but SDL 1 is obsolete too.
       dnl If problems occur in the future, this section can be removed.
       AS_IF([test "x$enable_sdl" != "x1"],
             [AS_IF([test "x$with_sdl2" = "xno"],
                    [AM_PATH_SDL([1.2.0], [enable_sdl=1], [enable_sdl=0])])])
       AC_SUBST([SDL_CFLAGS])
       AC_SUBST([SDL_LIBS])
       AS_IF([test "x$enable_sdl" != "x0"],
             [CFLAGS="$CFLAGS $SDL_CFLAGS"
              CXXFLAGS="$CXXFLAGS $SDL_CFLAGS"
              LIBS="$LIBS $SDL_LIBS"],
             [AC_MSG_FAILURE([SDL is required for graphical output and user interface])])])

dnl Use icon if present
AH_TEMPLATE([HAVE_ICON], [Set to 1 if window icon is compiled in and set by code.])
AC_CHECK_FILE([synaesthesia.png],
              [AS_IF([test "x$windows_host" = "xyes"],
                     [AC_CHECK_PROG([has_icotool],[icotool],[yes],[no])
                      AS_IF([test "x$has_icotool" = "xyes"],
                            [AC_CHECK_PROG([windows_icon],[windres],[yes],[no])],
                            [windows_icon=no])
                      AS_IF([test "x$windows_icon" != "xyes"],
                            [AC_MSG_WARN([Icon found but need icotool from icoutils and windres to use it])])],
                      [windows_icon=0])
               AS_IF([test "x$enable_emscripten" = "xyes"], [sdl_icon=no],
                     [test "x$windows_host" = "xyes" -a "x$with_sdl2" != "xno"], [sdl_icon=no],
                     [AC_CHECK_PROG([has_convert],[convert],[yes],[no])
                      AS_IF([test "x$has_convert" = "xyes"],
                            [AC_CHECK_PROG([has_xxd],[xxd],[yes],[no])],
                            [use_xxd=no])
                      AS_IF([test "x$has_xxd" = "xyes"],
                            [AC_DEFINE([HAVE_ICON], [1])
                             sdl_icon=yes],
                            [AC_MSG_WARN([Icon found but need convert from ImageMagick and xxd to use it])
                             sdl_icon=no])])])
AM_CONDITIONAL([COMPILE_WINICON], [test "x$windows_icon" = "xyes"])
AM_CONDITIONAL([COMPILE_SDLICON], [test "x$sdl_icon" = "xyes"])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
