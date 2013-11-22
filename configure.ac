AC_INIT(main.cc)

dnl Detect the canonical host and target build environment
AC_CANONICAL_SYSTEM

AM_INIT_AUTOMAKE(synaesthesia,2.4)
AM_CONFIG_HEADER(config.h)

CXXFLAGS="-O4"
CFLAGS="-O4"

AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL

dnl Check for X
AC_PATH_XTRA
if test "x$no_x" != xyes ; then  
  CFLAGS="$CFLAGS $X_CFLAGS"
  CXXFLAGS="$CXXFLAGS $X_CFLAGS"
  LIBS="$LIBS $X_PRE_LIBS $X_LIBS $X_EXTRA_LIBS -lX11"
fi

dnl Check for svgalib
AC_CHECK_LIB(vga,vga_init)

dnl Check for SDL
SDL_VERSION=1.2.0
AM_PATH_SDL($SDL_VERSION,
            AC_DEFINE(HAVE_SDL),
            :
)
CFLAGS="$CFLAGS $SDL_CFLAGS"
CXXFLAGS="$CXXFLAGS $SDL_CFLAGS"
LIBS="$LIBS $SDL_LIBS"

dnl Check for esound
AC_CHECK_LIB(esd,esd_record_stream_fallback)

AC_OUTPUT(Makefile)
