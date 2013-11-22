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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <pwd.h>
#include <string.h>
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

SymbolID state = NoCD;
int track = 1, frames = 0;
double trackProgress = 0.0;

char **playList;
int playListLength, playListPosition;

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

bool loadConfig() {
  setStateToDefaults();
  windX=10;
  windY=30;
  windWidth = DefaultWidth;
  windHeight = DefaultHeight;
  strcpy(dspName, "/dev/dsp");
  strcpy(mixerName, "/dev/mixer");
  strcpy(cdromName, "/dev/cdrom");

  //Should i free this? Manual is unclear
  struct passwd *passWord = getpwuid(getuid());
  if (passWord == 0) return false;

  char *fileName = new char[strlen(passWord->pw_dir) + 20];
  strcpy(fileName,passWord->pw_dir);
  strcat(fileName,"/.synaesthesia");
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
      sscanf(line,"dsp %s",dspName);
      sscanf(line,"mixer %s",mixerName);
      sscanf(line,"cdrom %s",cdromName);
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
  //Should i free this? Manual is unclear
  struct passwd *passWord = getpwuid(getuid());
  if (passWord == 0) {
    fprintf(stderr,"Couldn't work out where to put config file.\n");
    return;
  }
  
  char *fileName = new char[strlen(passWord->pw_dir) + 20];
  strcpy(fileName,passWord->pw_dir);
  strcat(fileName,"/.synaesthesia");
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

int main(int argc, char **argv) { 
  if (!loadConfig())
    saveConfig();

  if (argc == 1) {
    printf("SYNAESTHESIA " VERSION "\n\n"
           "Usage:\n"
           "  synaesthesia cd\n    - listen to a CD\n"
           "  synaesthesia line\n    - listen to line input\n"

#if HAVE_LIBESD
           "  synaesthesia esd\n    - listen to EsounD output (eg for use with XMMS)\n"
#endif
	   
           "  synaesthesia <track> <track> <track>...\n"
	   "    - play these CD tracks one after the other\n"
           "  <another program> |synaesthesia pipe <frequency>\n"
	   "    - send output of program to sound card as well as displaying it.\n"
	   "      (must be 16-bit stereo sound)\n"
	   "    example: mpg123 -s file.mp3 |synaesthesia pipe 44100\n\n"
	   "The following optional flags may be used\n"

#if HAVE_SDL
	   "     --use-sdl      force use of Simple DirectMedia Layer\n"
#endif
#ifndef X_DISPLAY_MISSING 
	   "     --use-x        force use of X-Windows\n"
#endif
#if HAVE_LIBVGA
	   "     --use-svga     force use of SVGALib\n"
#endif
	   "     --fullscreen   try to take over the whole screen\n"
	   "     --width nnn    make the window this wide\n"
	   "     --height nnn   make the window this high\n\n"
           "For more information, see http://logarithmic.net/pfh/Synaesthesia\n\n"
	   "Enjoy!\n"
	   );
    return 1;
  }

  //Do flags
  bool fullscreen = false, useSDL = true, useX = true, useSVGA = true;
  for(int i=0;i<argc;)
    if (strcmp(argv[i],"--use-sdl") == 0) {
      useX = false;
      useSVGA = false;
      chomp(argc,argv,i);
    } else if (strcmp(argv[i],"--use-x") == 0) {
      useSDL = false;
      useSVGA = false;
      chomp(argc,argv,i);
    } else if (strcmp(argv[i],"--use-svga") == 0) {
      useSDL = false;
      useX = false;
      chomp(argc,argv,i);
    } else if (strcmp(argv[i],"--fullscreen") == 0) {
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
    } else
      i++;

  int configPlayTrack = -1;
  int inFrequency = Frequency;

  playListLength = 0;
  
  if (strcmp(argv[1],"line") == 0) soundSource = SourceLine;
  else if (strcmp(argv[1],"cd") == 0) soundSource = SourceCD;

#if HAVE_LIBESD
  else if (strcmp(argv[1],"esd") == 0) soundSource = SourceESD;
#endif
  
  else if (strcmp(argv[1],"pipe") == 0) {
    if (argc < 3 || sscanf(argv[2],"%d",&inFrequency) != 1)
      error("frequency not specified");
    soundSource = SourcePipe;
  } else
    if (sscanf(argv[1],"%d",&configPlayTrack) != 1)
      error("comprehending user's bizzare requests");
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

  openSound(soundSource, inFrequency, dspName, mixerName); 
  
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

  screen = 0;

#if HAVE_SDL
  if (!screen && useSDL) {
    screen = new SdlScreen;
    if (!screen->init(windX,windY,windWidth,windHeight,fullscreen))
      screen = 0;
  }
#endif

#ifndef X_DISPLAY_MISSING
  if (!screen && useX) {
    screen = new XScreen;
    if (!screen->init(windX,windY,windWidth,windHeight,fullscreen))
      screen = 0;
  }
#endif

#if HAVE_LIBVGA
  if (!screen && useSVGA) {
    screen = new SvgaScreen;
    if (!screen->init(windX,windY,windWidth,windHeight,fullscreen))
      screen = 0;
  }
#endif

  if (!screen)
    error("opening any kind of display device");

  allocOutput(outWidth,outHeight);

  coreInit();

  setStarSize(starSize);

  interfaceInit();

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
  
  if (soundSource == SourceCD) 
    cdClose();

  closeSound();

  screen->end();
  delete screen;

  if (timer > 10)
    printf("Frames per second: %f\n", double(frames) / timer);
  
  return 0;
}

void error(char *str, bool syscall) { 
  fprintf(stderr, "synaesthesia: Error %s\n",str); 
  if (syscall)
    fprintf(stderr,"(reason for error: %s)\n",strerror(errno));
  exit(1);
}
void warning(char *str, bool syscall) { 
  fprintf(stderr, "synaesthesia: Possible error %s\n",str); 
  if (syscall)
    fprintf(stderr,"(reason for error: %s)\n",strerror(errno));
}

