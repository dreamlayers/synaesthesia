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

#include "config.h"
#include "syna.h"

/*
 * Functions called by JavaScript
 */

extern "C"
int displaySynaesthesia(int) {
  fade();

  coreGo();

  interfaceGo();

  screen->show();

  return 0;
}

extern "C"
float *getDataBuffer(void) {
  static sampleType dataBuffer[NumSamples*2];
  data = dataBuffer;
  data[0] = 1;
  return data;
}

/*
 * Functions called by Synaesthesia
 */

void openSound(SoundSource sound, int downFactor,
               char *dspName, char *mixerName) {
  (void)sound;
  (void)downFactor;
  (void)dspName;
  (void)mixerName;
}

void setupMixer(double &loudness) {
  loudness = 1.0;
}

void setVolume(double loudness) {
  (void)loudness;
}

int getNextFragment(void) {
  return 0;
}
