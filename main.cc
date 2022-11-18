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
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#if HAVE_PWD_H
#include <pwd.h>
#endif
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#define __W32API_USE_DLLIMPORT__
#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#endif
#include <string.h>
#include <SDL_main.h>
#include "syna.h"

//void setupIcons();

/* Config and globals =============================================== */

BaseScreen *screen;

sampleType *data;

int outWidth, outHeight;
Bitmap<unsigned short> outputBmp, lastOutputBmp, lastLastOutputBmp;

PolygonEngine<unsigned short,Combiner,2> polygonEngine;

void allocOutput(int w,int h) {
  /*delete[] output;
  delete[] lastOutput;
  delete[] lastLastOutput;
  output = new unsigned char[w*h*2];
  lastOutput = new unsigned char[w*h*2];
  lastLastOutput = new unsigned char[w*h*2];
  memset(output,32,w*h*2);
  memset(lastOutput,32,w*h*2);
  outWidth = w;
  outHeight = h;*/

  outputBmp.size(w,h);
  lastOutputBmp.size(w,h);
  lastLastOutputBmp.size(w,h);
  polygonEngine.size(w,h);
  outWidth = w;
  outHeight = h;
}

#ifdef HAVE_CD_PLAYER
SymbolID state = NoCD;
int track = 1, frames = 0;
double trackProgress = 0.0;

char **playList;
int playListLength, playListPosition;
#else /* !HAVE_CD_PLAYER */
SymbolID state = Plug;
#endif /* !HAVE_CD_PLAYER */

SymbolID fadeMode;
bool pointsAreDiamonds;

double brightnessTwiddler; 
double starSize; 
double volume = 0.5;

double fgRedSlider, fgGreenSlider, bgRedSlider, bgGreenSlider;

static SoundSource soundSource;

static int windX, windY, windWidth, windHeight;
static char dspName[80];
static char mixerName[80];
static char cdromName[80];
#ifdef MONITOR_NAME
static char monitorName[80];
#endif

void setStateToDefaults() {
  fadeMode = Stars;
  pointsAreDiamonds = true;

  brightnessTwiddler = 0.3;
  starSize = 0.5; 

  fgRedSlider=0.0;
  fgGreenSlider=0.5;
  bgRedSlider=0.75;
  bgGreenSlider=0.4;
}

char *getConfigFileName(void) {
#if defined(HAVE_GETPWUID) && defined(HAVE_GETUID)
  //Should i free this? Manual is unclear
  struct passwd *passWord = getpwuid(getuid());
  if (passWord == 0) return NULL;

  char *fileName = new char[strlen(passWord->pw_dir) + 20];
  strcpy(fileName,passWord->pw_dir);
  strcat(fileName,"/.synaesthesia");
#elif defined(WIN32)
  const char name[] = "synaesthesia.cfg";
  char *fileName = new char[MAX_PATH];
  if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL,
                                 SHGFP_TYPE_CURRENT, fileName))) {
    PathAppendA(fileName, name);
  } else {
    strcpy(fileName, name);
  }
#else
  char *fileName = new char[20];
  strcpy(fileName,".synaesthesia");
#endif
  return fileName;
}

bool loadConfig() {
  setStateToDefaults();
  windX=10;
  windY=30;
  windWidth = DefaultWidth;
  windHeight = DefaultHeight;
  strcpy(dspName, "/dev/dsp");
  strcpy(mixerName, "/dev/mixer");
  strcpy(cdromName, "/dev/cdrom");
#ifdef MONITOR_NAME
  strcpy(monitorName, MONITOR_NAME);
#endif

  char *fileName = getConfigFileName();
  if (fileName == NULL) return false;
  FILE *f = fopen(fileName,"rt");
  delete fileName;

  if (f) {
    char line[80];
    while(0 != fgets(line,80,f)) {
      sscanf(line,"x %d",&windX);
      sscanf(line,"y %d",&windY);
      sscanf(line,"width %d",&windWidth);
      sscanf(line,"height %d",&windHeight);
      sscanf(line,"brightness %lf",&brightnessTwiddler);
      sscanf(line,"pointsize %lf",&starSize);
      sscanf(line,"fgred %lf",&fgRedSlider);
      sscanf(line,"fggreen %lf",&fgGreenSlider);
      sscanf(line,"bgred %lf",&bgRedSlider);
      sscanf(line,"bggreen %lf",&bgGreenSlider);
      /* These are not buffer overflow risks because fgets() limits size */
      sscanf(line,"dsp %[^\r\n]",dspName);
      sscanf(line,"mixer %[^\r\n]",mixerName);
      sscanf(line,"cdrom %[^\r\n]",cdromName);
#ifdef MONITOR_NAME
      sscanf(line,"monitor %[^\r\n]",monitorName);
#endif
      if (strncmp(line,"fade",4) == 0) fadeMode = Stars;
      if (strncmp(line,"wave",4) == 0) fadeMode = Wave;
      if (strncmp(line,"heat",4) == 0) fadeMode = Flame;
      if (strncmp(line,"stars",5) == 0) pointsAreDiamonds = false;
      if (strncmp(line,"diamonds",8) == 0) pointsAreDiamonds = true;
    }
    fclose(f);
  
    if (windWidth < 1)
      windWidth = DefaultWidth;
    if (windHeight < 1)
      windHeight = DefaultHeight;
    windWidth  &= ~3;

#   define bound(v) \
      if (v < 0.0) v = 0.0; \
      if (v > 1.0) v = 1.0;

    bound(brightnessTwiddler)
    bound(starSize)
    bound(fgRedSlider)
    bound(fgGreenSlider)
    bound(bgRedSlider)
    bound(bgGreenSlider)

#   undef bound
  
    return true;
  } else
    return false;

}

void saveConfig() {
  char *fileName = getConfigFileName();
  if (fileName == NULL) {
    fprintf(stderr,"Couldn't work out where to put config file.\n");
    return;
  }
  FILE *f = fopen(fileName,"wt");
  delete fileName;

  if (f) {
    fprintf(f,"# Synaesthesia config file\n");
    fprintf(f,"x %d\n",windX);
    fprintf(f,"y %d\n",windY);
    fprintf(f,"width %d\n",outWidth);
    fprintf(f,"height %d\n",outHeight);

    fprintf(f,"# Point style: either diamonds or stars\n");
    fprintf(f,"%s\n",pointsAreDiamonds ? "diamonds" : "stars");
    
    fprintf(f,"# Fade style: either fade, wave or heat\n");
    fprintf(f,"%s\n",fadeMode==Stars ? "fade" : (fadeMode==Wave ? "wave" : "heat"));
    
    fprintf(f,"brightness %f\n",brightnessTwiddler);
    fprintf(f,"pointsize %f\n",starSize);
    fprintf(f,"fgred %f\n",fgRedSlider);
    fprintf(f,"fggreen %f\n",fgGreenSlider);
    fprintf(f,"bgred %f\n",bgRedSlider);
    fprintf(f,"bggreen %f\n",bgGreenSlider);
    fprintf(f,"dsp %s\n",dspName);
    fprintf(f,"mixer %s\n",mixerName);
    fprintf(f,"cdrom %s\n",cdromName);
#ifdef MONITOR_NAME
    fprintf(f,"monitor %s\n",monitorName);
#endif
    fclose(f);
  } else {
    fprintf(stderr,"Couldn't open config file to save settings.\n");
  }
}

void chomp(int &argc,char **argv,int argNum) {
  argc--;
  for(int i=argNum;i<argc;i++)
    argv[i] = argv[i+1];
}

#ifdef WINAMP
DWORD __stdcall synaesthesiaMain(LPVOID lpParameter)
#elif defined(AUDACIOUS)
void *synaesthesiaMain(void *arg)
#else
int main(int argc, char **argv)
#endif
{
  if (!loadConfig())
    saveConfig();

#if !defined(EMSCRIPTEN) && !defined(WINAMP) && !defined(AUDACIOUS)
  if (argc == 1) {
    printf("SYNAESTHESIA " VERSION "\n\n"
           "Usage:\n"

#ifdef MONITOR_NAME
           "  synaesthesia\n    - listen to computer playback and display help\n"
#endif

#ifdef HAVE_CD_PLAYER
           "  synaesthesia cd\n    - listen to a CD\n"
#endif /* HAVE_CD_PLAYER */

           "  synaesthesia line\n    - listen to line input\n"

#if HAVE_LIBESD
           "  synaesthesia esd\n    - listen to EsounD output (eg for use with XMMS)\n"
#endif
	   
#ifdef HAVE_CD_PLAYER
           "  synaesthesia <track> <track> <track>...\n"
	   "    - play these CD tracks one after the other\n"
#endif /* HAVE_CD_PLAYER */

#ifdef HAVE_PLAYBACK
           "  <another program> |synaesthesia pipe <frequency>\n"
	   "    - send output of program to sound card as well as displaying it.\n"
	   "      (must be 16-bit stereo sound)\n"
	   "    example: mpg123 -s file.mp3 |synaesthesia pipe 44100\n"
#endif /* HAVE_PLAYBACK */

	   "\nThe following optional flags may be used\n"
	   "     --classic      use original Synaesthesia 2.4 algorithms\n"
	   "     --fullscreen   try to take over the whole screen\n"
	   "     --width nnn    make the window this wide\n"
	   "     --height nnn   make the window this high\n\n"
           "For more information, see http://logarithmic.net/pfh/Synaesthesia\n\n"
	   "Enjoy!\n"
	   );
#ifndef MONITOR_NAME
    return 1;
#endif
  }
#endif

  //Do flags
  bool fullscreen = false;
  bool logfreq = true, hamming = true, truecolor = true, addsq = true;
#if !defined(WINAMP) && !defined(AUDACIOUS)
  for(int i=1;i<argc;)
    if (strcmp(argv[i],"--fullscreen") == 0) {
      fullscreen = true;
      chomp(argc,argv,i);
    } else if (strcmp(argv[i],"--width") == 0) {
      chomp(argc,argv,i);
      windWidth = atoi(argv[i]);
      if (windWidth < 1)
        windWidth = DefaultWidth;
      windWidth  &= ~3;
      chomp(argc,argv,i);
    } else if (strcmp(argv[i],"--height") == 0) {
      chomp(argc,argv,i);
      windHeight = atoi(argv[i]);
      if (windHeight < 1)
        windHeight = DefaultHeight;
      chomp(argc,argv,i);
    } else if (strcmp(argv[i],"--classic") == 0) {
      logfreq = false;
      hamming = false;
      truecolor = false;
      addsq = false;
      chomp(argc,argv,i);
    } else
      i++;
#endif /* !WINAMP && !AUDACIOUS */

#ifdef HAVE_CD_PLAYER
  int configPlayTrack = -1;
#endif
  int inFrequency = Frequency;

#ifdef HAVE_CD_PLAYER
  playListLength = 0;
#endif /* HAVE_CD_PLAYER */

#if !defined(EMSCRIPTEN) && !defined(WINAMP) && !defined(AUDACIOUS)
#ifdef MONITOR_NAME
  if (argc == 1) soundSource = SourceMonitor;
  else
#endif
  if (strcmp(argv[1],"line") == 0) soundSource = SourceLine;
#ifdef HAVE_CD_PLAYER
  else if (strcmp(argv[1],"cd") == 0) soundSource = SourceCD;
#endif /* HAVE_CD_PLAYER */

#if HAVE_LIBESD
  else if (strcmp(argv[1],"esd") == 0) soundSource = SourceESD;
#endif

#ifdef HAVE_PLAYBACK
  else if (strcmp(argv[1],"pipe") == 0) {
    if (argc < 3 || sscanf(argv[2],"%d",&inFrequency) != 1)
      error("frequency not specified");
    soundSource = SourcePipe;
  }
#endif
    else
#ifdef HAVE_CD_PLAYER
    if (sscanf(argv[1],"%d",&configPlayTrack) != 1)
#endif /* HAVE_CD_PLAYER */
      error("comprehending user's bizzare requests");

#ifdef HAVE_CD_PLAYER
    else {
      soundSource = SourceCD;
      playList = argv+1;
      playListPosition = 0;
      playListLength = argc-1;
    }

  if (soundSource == SourceCD)
    cdOpen(cdromName);
  else
    state = Plug;

  if (configPlayTrack != -1) {
    cdStop();
  }
#endif /* HAVE_CD_PLAYER */
#else /* EMSCRIPTEN */
    soundSource = SourceLine;
#endif

  openSound(soundSource, inFrequency,
#ifdef MONITOR_NAME
            (soundSource == SourceMonitor) ? monitorName :
#endif
            dspName, mixerName);
  
  setupMixer(volume);

  //if (volume > 0.0) {
  //  brightnessTwiddler /= volume;
  //}
  //if (brightnessTwiddler > 1.0)
  //  brightnessTwiddler = 1.0;
  //else if (brightnessTwiddler < 0.0)
  //  brightnessTwiddler = 0.0;

  //if (volume > 1.0)
  //  volume = 1.0;
  //else if (volume < 0.0)
  //  volume = 0.0;

  screen = new SdlScreen;
  if (!screen->init(windX,windY,windWidth,windHeight,fullscreen,
                    truecolor ? 32 : 8))
    screen = 0;

  if (!screen)
    error("opening any kind of display device");

  allocOutput(outWidth,outHeight);

  coreInit(logfreq, hamming, addsq);

  setStarSize(starSize);

  interfaceInit();

/* With Emscripten, main() returns after initialization, and
 * further calls come from JavaScript via displaySynaesthesia().
 */
#ifndef EMSCRIPTEN
  time_t timer = time(NULL);
  
  int frames = 0;
  for(;;) {
    fade();

    if (-1 == coreGo())
      break;

    if (interfaceGo()) break;
    
    screen->show(); 

    frames++;
  } 

  timer = time(NULL) - timer;

  interfaceEnd();
  
//  if (configPlayTrack != -1)
//    cdStop();    
  
#ifdef HAVE_CD_PLAYER
  if (soundSource == SourceCD) 
    cdClose();
#endif /* HAVE_CD_PLAYER */

  closeSound();

  screen->end();
  delete screen;

  if (timer > 10)
    printf("Frames per second: %f\n", double(frames) / timer);
#endif /* !EMSCRIPTEN */
  return 0;
}

void error(const char *str, bool syscall) {
  fprintf(stderr, "synaesthesia: Error %s\n",str); 
  if (syscall)
    fprintf(stderr,"(reason for error: %s)\n",strerror(errno));
  exit(1);
}
void warning(const char *str, bool syscall) {
  fprintf(stderr, "synaesthesia: Possible error %s\n",str); 
  if (syscall)
    fprintf(stderr,"(reason for error: %s)\n",strerror(errno));
}

