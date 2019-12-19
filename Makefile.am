bin_PROGRAMS = synaesthesia

synaesthesia_SOURCES = main.cc core.cc ui.cc sdlwrap.cc

if COMPILE_UNIXSOUND
synaesthesia_SOURCES += sound.cc
endif

if COMPILE_WINSOUND
synaesthesia_SOURCES += winsound.cc
endif

if COMPILE_WINDOWS
synaesthesia_LDFLAGS = -static

if COMPILE_WINICON
synaesthesia_SOURCES += syn-res.rc
synaesthesia_DEPENDENCIES = synaesthesia.ico

.rc.o:
	windres -o $@ $<

syn-res.o: synaesthesia.ico

synaesthesia.ico: synaesthesia.png
	icotool -c -o $@ $^
endif

if COMPILE_WINAMP
synaesthesia_SOURCES += winamp.cc
synaesthesia_LDFLAGS += -shared -Wl,-no-undefined
endif
endif COMPILE_WINDOWS

if COMPILE_SDLICON
syn_icon.rgb: synaesthesia.png
	convert $< -resize "64x64!" $@

syn_icon_rgb.cc: syn_icon.rgb
	xxd -i $< > $@

nodist_synaesthesia_SOURCES = syn_icon_rgb.cc
endif COMPILE_SDLICON

if COMPILE_AUDACIOUS
synaesthesia_SOURCES += audacious.cc
synaesthesia_LDFLAGS = -shared -Wl,-no-undefined
AM_CXXFLAGS = $(AUDACIOUS_CFLAGS)
synaesthesia_LDADD = $(AUDACIOUS_LIBS)
endif

if COMPILE_PORTAUDIO
synaesthesia_SOURCES += pawrap.cc soundbuf.cc
endif

if COMPILE_EMSCRIPTEN
synaesthesia_SOURCES += emscripten.cc
synaesthesia_LDFLAGS = -s EXPORTED_FUNCTIONS="['_main', '_getDataBuffer', '_displaySynaesthesia']" -s NO_EXIT_RUNTIME=1
synaesthesia_LDFLAGS += --post-js soundplayer.js
synaesthesia_DEPENDENCIES = soundplayer.js
endif

CLEANFILES = synaesthesia.ico syn_icon.rgb syn_icon_rgb.cc \
             synaesthesia.js synaesthesia.html.mem a.out.js

install-data-local:
	-xdg-icon-resource install --novendor --context apps \
	                           --size 256 synaesthesia.png synaesthesia
	-xdg-desktop-menu install --novendor synaesthesia.desktop

uninstall-local:
	-xdg-icon-resource uninstall --novendor --context apps \
	                             --size 256 synaesthesia
	-xdg-desktop-menu uninstall --novendor synaesthesia.desktop
