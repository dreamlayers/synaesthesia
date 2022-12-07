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
#include <string.h>
#include <portaudio.h>
#include "syna.h"

PaStream *stream;

void setupMixer(double &loudness) {
}

void setVolume(double loudness) {
}

static int pa_callback(const void *in, void *out,
                       unsigned long frameCount,
                       const PaStreamCallbackTimeInfo *timeInfo,
                       PaStreamCallbackFlags statusFlags, void *userData) {
    sndbuf_store((int16_t *)in, frameCount * 2);
    return paContinue;
}

void openSound(SoundSource source, int inFrequency, char *dspName,
               char *mixerName, unsigned int chunk_size) {
  PaError err;

  err = Pa_Initialize();
  if(err != paNoError) error("initializing PortAudio.");

  sndbuf_init(chunk_size * 2);

  PaStreamParameters inputParameters;

  /* Search through devices to find one matching dspName.
     However, don't use /dev/dsp via Portaudio. It works badly,
     just like when you're using it directly. Excluding it
     because it will be the default in the configuration file. */
  if (dspName != NULL && strcmp(dspName, "/dev/dsp")) {
    PaDeviceIndex numDevices = Pa_GetDeviceCount();
#ifdef WIN32
    unsigned int namelen = strlen(dspName);
#endif
    for (inputParameters.device = 0; inputParameters.device < numDevices;
         inputParameters.device++) {
      const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(inputParameters.device);
      if (deviceInfo != NULL && deviceInfo->name != NULL &&
#ifdef WIN32
          /* Allow substring matches, for eg. "Stereo Mix (Sound card name)" */
          !strncmp(deviceInfo->name, dspName, namelen)
#else
          !strcmp(deviceInfo->name, dspName)
#endif
          ) break;
    }
    if (inputParameters.device == numDevices) {
      warning("couldn't find selected sound device, using default");
      inputParameters.device = Pa_GetDefaultInputDevice();
    }
  } else {
    inputParameters.device = Pa_GetDefaultInputDevice();
  }
  if (inputParameters.device == paNoDevice) {
    error("couldn't find default sound input device");
  }

  inputParameters.channelCount = 2;
  inputParameters.sampleFormat = paInt16; /* PortAudio uses CPU endianness. */
  inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
  inputParameters.hostApiSpecificStreamInfo = NULL;
  err = Pa_OpenStream(&stream,
                      &inputParameters,
                      NULL, //&outputParameters,
                      inFrequency,
                      chunk_size,
                      paClipOff,
                      pa_callback,
                      NULL); /* no callback userData */
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

  sndbuf_quit();

  delete data;
}
