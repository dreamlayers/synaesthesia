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

/* This is winamp.cc, for interfacing Synaesthesia to Winamp as a plugin
   Parts taken from Synaesthesia's sound.cc and Winamp example visualization
   plugin svis.c
   Copyright (C) 2008  Boris Gjenero
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "syna.h"

// Can use either older frontend.h or newer wa_ipc.h
//#include "frontend.h"
#include "wa_ipc.h"
#include "vis.h"

// Thread which does all the work
HANDLE mainThread;
// Inter-thread signalling:
// Winamp interface thread telling Synaesthesia thread that data is available
HANDLE haveData = 0;
// Synaesthesia thread telling Winamp thread data was used
HANDLE gotData;
// Signals from thread: { mainThread, gotData }
HANDLE fromThread[2];
// Normally 0, set to -1 to quit
int quitFlag = -1;

// Buffer for real samples and zeroes
sampleType realdata[2*NumSamples], fakedata[2*NumSamples];

// Configuration function which does nothing
void config(struct winampVisModule *this_mod) {
  MessageBox(this_mod->hwndParent,
             "Synaesthesia 2.4 by Paul Francis Harrison\n"
             "Winamp plugin port by Boris Gjenero\n"
             "To configure, start the plugin and use its interface.\n"
             "Remember to save the configuration if you want to keep it.\n",
             "Synaesthesia 2.4 for Winamp", MB_OK);
}

int init(struct winampVisModule *this_mod);
int render(struct winampVisModule *this_mod);
void quit(struct winampVisModule *this_mod);

char description[] = "Synaesthesia 2.4";

winampVisModule mod1 = {
  description,
  NULL,   // hwndParent
  NULL,   // hDllInstance
  0,      // sRate
  0,      // nCh
  25,     // latencyMS
  25,     // delayMS
  0,      // spectrumNch
  2,      // waveformNch
  { 0, }, // spectrumData
  { 0, }, // waveformData
  config,
  init,
  render,
  quit
};

winampVisModule *getModule(int which) {
  switch (which)
  {
    case 0:  return &mod1;
    default: return NULL;
  }
}

winampVisHeader hdr = { VIS_HDRVER, description, getModule };

extern "C" __declspec(dllexport) winampVisHeader *winampVisGetHeader() {
  return &hdr;
}

void cdClose(void) {
}

// Normally used to poll the CD drive
// Instead, information is retrieved from Winamp when specifically requested
void getTrackInfo(void) {
}

int cdGetTrackCount(void) {
  return SendMessage(mod1.hwndParent, WM_WA_IPC, 0, IPC_GETLISTLENGTH);
}

int cdGetTrackFrame(int track) {
  return (track - 1) << 8;
}

void cdPlay(int frame, int endFrame) {
  int track = frame >> 8;

  // If we do play there is a race condition in Winamp and the SEEKTOPOS will be ignored.
  // Therefore only do a play if the track needs to be changed or if we need to start playing.
  if (track != SendMessage(mod1.hwndParent, WM_WA_IPC, 0, IPC_GETLISTPOS)
      || SendMessage(mod1.hwndParent, WM_WA_IPC, 0, IPC_ISPLAYING) != 1) {
    SendMessage(mod1.hwndParent, WM_WA_IPC, track, IPC_SETPLAYLISTPOS);
    // The play is needed to actually start playing the new track or to play if we weren't playing.
    SendMessage(mod1.hwndParent, WM_COMMAND, WINAMP_BUTTON2, 0);
  }

  // Seek to desired position.
  SendMessage(mod1.hwndParent, WM_WA_IPC, (frame & 0xff) *
              SendMessage(mod1.hwndParent, WM_WA_IPC, 1, IPC_GETOUTPUTTIME)
              * 1000 / 255, IPC_JUMPTOTIME);
}

void cdGetStatus(int &track, int &frames, SymbolID &state) {
  switch (SendMessage(mod1.hwndParent, WM_WA_IPC, 0, IPC_ISPLAYING)) {
    case 0: state = Stop; break;
    case 1: state = Play; break;
    case 3: state = Pause; break;
  }

  track = SendMessage(mod1.hwndParent, WM_WA_IPC, 0, IPC_GETLISTPOS) + 1;

  int tframes = SendMessage(mod1.hwndParent, WM_WA_IPC, 1, IPC_GETOUTPUTTIME);
  if (tframes != 0) {
    frames = SendMessage(mod1.hwndParent, WM_WA_IPC, 0, IPC_GETOUTPUTTIME)
    * 255 / tframes / 1000;
  } else {
    frames = 0;
  }
}

void cdStop(void) {
  SendMessage(mod1.hwndParent, WM_COMMAND, WINAMP_BUTTON4, 0);
}

void cdPause(void) {
  SendMessage(mod1.hwndParent, WM_COMMAND, WINAMP_BUTTON3, 0);
}

void cdResume(void) {
  SendMessage(mod1.hwndParent, WM_COMMAND, WINAMP_BUTTON2, 0);
}

void cdEject(void) {
  PostMessage(mod1.hwndParent, WM_COMMAND, WINAMP_FILE_PLAY, 0);
}

void cdCloseTray(void) {
}

/* Sound Recording ================================================= */

void setupMixer(double &loudness) {
  // There doesn't seem to be a way to get Winamp's volume
}

void setVolume(double loudness) {
  SendMessage(mod1.hwndParent, WM_WA_IPC, (int)(loudness * 255), IPC_SETVOLUME);
}

void openSound(SoundSource source, int inFrequency, char *dspName,
               char *mixerName) {
  data = realdata;
  memset(fakedata, 0, NumSamples * 2 * sizeof(sampleType));
}

void closeSound() {
  data = NULL;
}

int getNextFragment(void) {
  // It's easiest to do this here.
  // It decreases lag but may also decrease frame rate
  if (data == realdata) {
    SetEvent(gotData);
  }

  // Wait for data
  data = (WaitForSingleObject(haveData, 1000/10) == WAIT_TIMEOUT) ?
    // Timeout, insert silent frame to keep interface responsive
    fakedata :
    // Either we have data or are told to quit
    realdata;

  // Calling function will either take data or quit
  return quitFlag;
}

// Not in syna.h so windows.h doesn't have to be included everywhere.
DWORD __stdcall synaesthesiaMain(LPVOID lpParameter);

int init(struct winampVisModule *this_mod) {
  // inits while running should not happen
  if (quitFlag != -1)
    return 1;

  haveData = CreateEvent(NULL, FALSE, FALSE, NULL);
  if (haveData == NULL)
    return 1;
  gotData = CreateEvent(NULL, FALSE, FALSE, NULL);
  if (gotData == NULL) {
    CloseHandle(haveData);
    return 1;
  }

  quitFlag = 0;

  mainThread = CreateThread(NULL, 0, synaesthesiaMain, NULL, 0, NULL);
  if (mainThread == NULL) {
    CloseHandle(haveData);
    CloseHandle(gotData);
    return 1;
  }

  fromThread[0] = mainThread;
  fromThread[1] = gotData;

  // Either thread signals it's ready or it fails and quits
  if (WaitForMultipleObjects(2, fromThread, FALSE, INFINITE)
      == WAIT_OBJECT_0) {
    // Thread quit
    CloseHandle(haveData);
    CloseHandle(gotData);
    return 1;
  }

  return 0;
}

int render(struct winampVisModule *this_mod) {
  // render should not happen while not running
  if (quitFlag != 0)
    return 1;

  // realdata must be free because we later wait until gotData

  // Copy data from Winamp to Synaesthesia structure
  if (this_mod->nCh >= 2) {
    for(int i=0; i<NumSamples; i++) {
      realdata[i*2] = ((signed char *)(this_mod->waveformData[0]))[i] << 8;
      realdata[i*2+1] = ((signed char *)(this_mod->waveformData[1]))[i] << 8;
    }
  } else if (this_mod->nCh == 1) {
    // Synaesthesia is not designed for mono
    for(int i=0; i<NumSamples; i++) {
      realdata[i*2] = ((signed char *)(this_mod->waveformData[0]))[i] << 8;
      realdata[i*2+1] = data[i*2];
    }
  } else {
    // 0 channels?  This shouldn't happen.
    return 1;
  }

  // Tell thread it has data
  SetEvent(haveData);

  // Wait for thread to finish with data
  if (WaitForMultipleObjects(2, fromThread, FALSE, INFINITE) == WAIT_OBJECT_0) {
    // Thread died, quit
    quitFlag = -1;
    return 1;
  }

  return 0;
}

void quit(struct winampVisModule *this_mod) {
  if (quitFlag == 0) {
    // Tell thread to quit and wait for it
    quitFlag = -1;
    SetEvent(haveData);
    WaitForSingleObject(mainThread, INFINITE);
  }

  if (haveData != 0) {
    CloseHandle(haveData);
    CloseHandle(gotData);
    CloseHandle(mainThread);
    haveData = 0;
  }
}

#if 0
// For testing only
extern "C" int main() {
  while (1) {
    init(&mod1); while (!render(&mod1)); quit(&mod1);
  }
  return 0;
}
#endif
