/* Synaesthesia - program to display sound graphically
   Copyright (C) 1997  Paul Francis Harrison
   Copyright (C) 2022  Boris Gjenero

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the
  Free Software Foundation; either version 2 of the License, or (at your
  option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  675 Mass Ave, Cambridge, MA 02139, USA.

  The author may be contacted at:
    pfh@yoyo.cc.monash.edu.au
  or
    27 Bond St., Mt. Waverley, 3149, Melbourne, Australia
*/

#include <stdint.h>
#include <string.h>
#include <SDL.h>
#include "syna.h"

static SDL_AudioDeviceID id = 0;

void setupMixer(double &loudness) {
}

void setVolume(double loudness) {
}

static void audioCallback(void *userdata, Uint8 * stream, int len) {
  sndbuf_store((int16_t *)stream, len / 2);
}

void openSound(SoundSource source, int inFrequency, char *dspName,
               char *mixerName) {
  if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0)
    sdlError("initializing SDL audio");

  sndbuf_init();

  SDL_AudioSpec desired, obtained;
  memset(&desired, 0, sizeof(desired));
  desired.freq = inFrequency;
#ifdef LITTLEENDIAN
  desired.format = AUDIO_S16LSB;
#else
  desired.format = AUDIO_S16MSB;
#endif
  desired.channels = 2;
  /* This counts samples in both channels */
  desired.samples = NumSamples / 2;
  desired.callback = audioCallback;

  id = SDL_OpenAudioDevice(NULL, 1, &desired, &obtained, 0);
  if (id <= 0)
    sdlError("opening SDL audio");

  SDL_PauseAudioDevice(id, 0);

  data = new int16_t[NumSamples*2];
  memset((char*)data,0,NumSamples*4);
}

void closeSound() {
  SDL_PauseAudioDevice(id, 1);

  SDL_CloseAudioDevice(id);
  id = 0;

  sndbuf_quit();

  delete data;
}
