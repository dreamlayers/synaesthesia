
bin_PROGRAMS = synaesthesia

synaesthesia_SOURCES = main.cc core.cc sound.cc ui.cc

if COMPILE_SVGALIB
synaesthesia_SOURCES += svgawrap.cc
endif

if COMPILE_SDL
synaesthesia_SOURCES += sdlwrap.cc
endif

if COMPILE_X11
synaesthesia_SOURCES += xlibwrap.cc xlib.c
endif
