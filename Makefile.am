bin_PROGRAMS = synaesthesia

synaesthesia_SOURCES = main.cc core.cc ui.cc

if COMPILE_UNIXSOUND
synaesthesia_SOURCES += sound.cc
endif

if COMPILE_SDL
synaesthesia_SOURCES += sdlwrap.cc
endif

if COMPILE_WINDOWS
synaesthesia_SOURCES += winsound.cc
synaesthesia_LDFLAGS = -static-libgcc -static-libstdc++
endif

if COMPILE_WINAMP
synaesthesia_SOURCES += winamp.cc
synaesthesia_LDFLAGS = -static-libgcc -static-libstdc++ \
                       -shared -Wl,-no-undefined
endif

if COMPILE_PORTAUDIO
synaesthesia_SOURCES += pawrap.cc
endif

if COMPILE_EMSCRIPTEN
synaesthesia_SOURCES += emscripten.cc
synaesthesia_LDFLAGS = -s EXPORTED_FUNCTIONS="['_main', '_getDataBuffer', '_displaySynaesthesia']"
synaesthesia_LDFLAGS += --post-js soundplayer.js
synaesthesia_DEPENDENCIES = soundplayer.js
endif
