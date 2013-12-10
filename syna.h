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
    phar6@student.monash.edu.au
  or
    27 Bond St., Mt. Waverley, 3149, Melbourne, Australia
*/

#include "config.h"

#include "polygon.h"

/***************************************/
/*   For the incurably fiddle prone:   */

/* log2 of sample size */
#define LogSize 10 //was 9 

/* Brightness */
#define Brightness 150

/* Sample frequency*/
#define Frequency 22050

#define DefaultWidth  200
#define DefaultHeight 200

/***************************************/

#define NumSamples (1<<LogSize)
#define RecSize (1<<LogSize-Overlap)

#ifdef __FreeBSD__

typedef unsigned short sampleType;

#else

typedef short sampleType;

#ifndef __linux__

#warning This target has not been tested!

#endif
#endif

#if !defined(BIGENDIAN) && !defined(LITTLEENDIAN)
#ifdef __FreeBSD__
#include <machine/endian.h>
#else
#include <endian.h>
#endif

#if BYTE_ORDER == BIG_ENDIAN
#define BIGENDIAN
#else
#define LITTLEENDIAN
#endif
#endif /* !defined(BIGENDIAN) && !defined(LITTLEENDIAN) */

void error(const char *str,bool syscall=false);
void inline attempt(int x,const char *y,bool syscall=false) { if (x == -1) error(y,syscall); }
void warning(const char *str,bool syscall=false);
void inline attemptNoDie(int x,const char *y,bool syscall=false) { if (x == -1) warning(y,syscall); }

/* *wrap */
struct BaseScreen {
  virtual bool init(int xHint, int yHint, int widthHint, int heightHint,
                    bool fullscreen, int depth = 8) = 0;
  virtual void setPalette(unsigned char *palette) = 0;
  virtual void end() = 0;
  virtual void inputUpdate(int &mouseX,int &mouseY,int &mouseButtons,char &keyHit) = 0;
  virtual void show() = 0;
  virtual void toggleFullScreen() { }
  virtual int getDepth() { return 8; }
};

struct SvgaScreen : public BaseScreen {
  bool init(int xHint, int yHint, int widthHint, int heightHint,
            bool fullscreen, int depth);
  void setPalette(unsigned char *palette);
  void end();
  void inputUpdate(int &mouseX,int &mouseY,int &mouseButtons,char &keyHit);
  void show();
};

struct SdlScreen : public BaseScreen {
  bool init(int xHint, int yHint, int widthHint, int heightHint,
            bool fullscreen, int depth);
  void setPalette(unsigned char *palette);
  void end();
  void inputUpdate(int &mouseX,int &mouseY,int &mouseButtons,char &keyHit);
  void show();
  void toggleFullScreen();
  int getDepth();
};

struct XScreen : public BaseScreen {
  bool init(int xHint, int yHint, int widthHint, int heightHint,
            bool fullscreen, int depth);
  void setPalette(unsigned char *palette);
  void end();
  void inputUpdate(int &mouseX,int &mouseY,int &mouseButtons,char &keyHit);
  void show();
};

/* core */
extern BaseScreen *screen;
extern sampleType *data;
extern Bitmap<unsigned short> outputBmp, lastOutputBmp, lastLastOutputBmp;
#define output ((unsigned char*)outputBmp.data)
#define lastOutput ((unsigned char*)lastOutputBmp.data)
#define lastLastOutput ((unsigned char*)lastLastOutputBmp.data)

struct Combiner {
  static unsigned short combine(unsigned short a,unsigned short b) {
    //Not that i want to give the compiler a hint or anything...
    unsigned char ah = a>>8, al = a&255, bh = b>>8, bl = b&255;
    if (ah < 64) ah *= 4; else ah = 255;
    if (al < 64) al *= 4; else al = 255;
    if (bh > ah) ah = bh;
    if (bl > al) al = bl;
    return ah*256+al;
  }
};

extern PolygonEngine<unsigned short,Combiner,2> polygonEngine;

extern int outWidth, outHeight;

void allocOutput(int w,int h);

void coreInit(bool logfreq, bool window);
void setStarSize(double size);
int coreGo();
void fade();

/* ui */
void interfaceInit();
void interfaceSyncToState();
void interfaceEnd();
bool interfaceGo();

enum SymbolID {
  Speaker, Bulb,
  Play, Pause, Stop, SkipFwd, SkipBack,
  Handle, Pointer, Open, NoCD, Exit,
  Zero, One, Two, Three, Four,
  Five, Six, Seven, Eight, Nine,
  Slider, Selector, Plug, Loop, Box, Bar,
  Flame, Wave, Stars, Star, Diamond, Size, FgColor, BgColor,
  Save, Reset, TrackSelect,
  NotASymbol
};

/* State information */

extern SymbolID state;
#ifdef HAVE_CD_PLAYER
extern int track, frames;
extern double trackProgress;
extern char **playList;
extern int playListLength, playListPosition;
#endif /* HAVE_CD_PLAYER */
extern SymbolID fadeMode;
extern bool pointsAreDiamonds;
extern double brightnessTwiddler; 
extern double starSize; 
extern double fgRedSlider, fgGreenSlider, bgRedSlider, bgGreenSlider;

extern double volume; 

void setStateToDefaults();
void saveConfig();

void putString(char *string,int x,int y,int red,int blue);

/* sound */
enum SoundSource {
  SourceLine,
#ifdef HAVE_CD_PLAYER
  SourceCD,
#endif /* HAVE_CD_PLAYER */
  SourcePipe,
#ifdef HAVE_LIBESD
  SourceESD,
#endif /* HAVE_LIBESD */
};

#ifdef HAVE_CD_PLAYER
void cdOpen(char *cdromName);
void cdClose(void);
void cdGetStatus(int &track, int &frames, SymbolID &state);
void cdPlay(int trackFrame, int endFrame=-1); 
void cdStop(void);
void cdPause(void);
void cdResume(void);
void cdEject(void);
void cdCloseTray(void);
int cdGetTrackCount(void);
int cdGetTrackFrame(int track);
#endif /* HAVE_CD_PLAYER */
void openSound(SoundSource sound, int downFactor, char *dspName, char *mixerName);
void closeSound();
void setupMixer(double &loudness);
void setVolume(double loudness);
int getNextFragment(void);

