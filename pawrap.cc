/* Synaesthesia - program to display sound graphically
   Copyright (C) 1997  Paul Francis Harrison

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
#include <portaudio.h>
#include "syna.h"

PaStream *stream;

void setupMixer(double &loudness) {
}

void setVolume(double loudness) {
}

void openSound(SoundSource source, int inFrequency, char *dspName,
               char *mixerName) {
  PaError err;

  err = Pa_Initialize();
  if(err != paNoError) error("initializing PortAudio.");

  PaStreamParameters inputParameters;
  inputParameters.device = Pa_GetDefaultInputDevice();
  inputParameters.channelCount = 2;
  inputParameters.sampleFormat = paInt16; /* PortAudio uses CPU endianness. */
  inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
  inputParameters.hostApiSpecificStreamInfo = NULL;
  err = Pa_OpenStream(&stream,
                      &inputParameters,
                      NULL, //&outputParameters,
                      inFrequency,
                      NumSamples,
                      paClipOff,
                      NULL, /* no callback, use blocking API */
                      NULL); /* no callback, so no callback userData */
  if(err != paNoError) error("opening PortAudio stream");

  err = Pa_StartStream( stream );
  if(err != paNoError) error("starting PortAudio stream");

  data = new int16_t[NumSamples*2];
  memset((char*)data,0,NumSamples*4);
}

void closeSound() {
  PaError err;

  err = Pa_StopStream(stream);
  if (err != paNoError) error("stopping PortAudio stream");

  err = Pa_CloseStream(stream);
  if (err != paNoError) error("closing PortAudio stream");

  err = Pa_Terminate();
  if (err != paNoError) error("terminating PortAudio");

  delete data;
}

int getNextFragment(void) {
  PaError err;
  err = Pa_ReadStream(stream, data, NumSamples);
  if (err != paNoError && err != paInputOverflowed)
    error("reading from PortAudio stream");
  return 0;
}
