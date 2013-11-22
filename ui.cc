#include "font.h"
#include "icons.h"
#include "syna.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

void putChar(unsigned char character,int x,int y,int red,int blue) {
  unsigned short *ptr = ((unsigned short *)output) + x + y*outWidth;
  unsigned short  put = (blue<<8)+red;
  int i,j;
  for(i=0;i<8;i++,ptr += outWidth-8)
    for(j=0;j<8;j++,ptr++)
      if (font[character*8+i] & (128>>j))
        *ptr = put;
}

void putString(char *string,int x,int y,int red,int blue) {
  if (x < 0 || y < 0 || y >= outHeight-8)
    return;
  for(;*string && x <= outWidth-8;string++,x+=8)
    putChar((unsigned char)(*string),x,y,red,blue);
}

struct UIObject {
  UIObject *next;

  int visibleMask, activeMask;
  double x,y,width,height;
  bool active;

  virtual int go(bool mouseDown,bool mouseClick,bool mouseOver,
                  double x, double y, double scale, char &hotKey, int &action)
		  = 0;

  virtual void handleKey(char key, int &action) = 0;
};

struct Button : public UIObject {
  int icon;
  char hotKey;

  bool passive, bright;

  Button(int _am,int _vm,double _x,double _y,
         double _size,int _icon,char _key = 0, 
	 bool _passive = false, bool _bright = false) {
    activeMask = _am; visibleMask = _vm;
    x = _x; y = _y; width = height = _size;
    icon = _icon; hotKey = _key; passive = _passive; bright = _bright;
  }
  
  int go(bool mouseDown,bool mouseClick,bool mouseOver,
          double _x, double _y, double scale, char &_hotKey, int &action) {
    polygonEngine.icon(
	Icons[icon],
	(bright ? 0x0404 : 0x0200),
	x*scale,y*scale,width*scale,height*scale);

    if (mouseOver && !passive)
      polygonEngine.icon(
  	  Icons[icon],
	  0x0004,
	  (x-IconWidths[icon]*width/2)*scale,
	  (y-height/2)*scale,width*scale*2,height*scale*2);

    if (mouseOver && mouseClick && !passive)
      action = icon;

    if (mouseOver && !passive && hotKey)
      _hotKey = hotKey;

    return 0;
  }
  
  void handleKey(char key, int &action) {
    if (key == hotKey && !passive)
      action = icon;
  }
};

struct PopperUpper : public UIObject {
  int maskAdd;

  PopperUpper(int _am,int _vm,double _x,double _y,
         double _width,double _height, int _maskAdd) {
    activeMask = _am; visibleMask = _vm; 
    x = _x; y = _y; width = _width; height = _height;
    maskAdd = _maskAdd;
  }

  int go(bool mouseDown,bool mouseClick,bool mouseOver,
          double _x, double _y, double scale, char &_hotKey, int &action) {
    polygonEngine.icon(
	Icons[Box],
	0x0200,
	x*scale,y*scale,width*scale,height*scale);

    return mouseOver ? maskAdd : 0;
  }
  
  void handleKey(char key, int &action) { }
};
 
#define BarWidth 0.1
struct SliderBar : public UIObject {
  double *value;
  char leftKey, rightKey;

  typedef void (*Callback)(double v);
  Callback callback;

  SliderBar(int _am,int _vm,double _x,double _y,
         double _width,double _height, double *_value, 
	 Callback _callback, char _leftKey, char _rightKey) {
    activeMask = _am; visibleMask = _vm; 
    x = _x; y = _y; width = _width; height = _height;
    value = _value; callback = _callback;
    leftKey = _leftKey; rightKey = _rightKey;
  }

  int go(bool mouseDown,bool mouseClick,bool mouseOver,
          double _x, double _y, double scale, char &_hotKey, int &action) {
    polygonEngine.icon(
      Icons[Bar],
      0x0100,
      x*scale,y*scale,width*scale,height*scale);
    polygonEngine.icon(
      Icons[Slider],
      0x0400,
      (x+*value*width-IconWidths[Slider]*height/2)*scale,
      y*scale,height*scale,height*scale);

    if (mouseOver) {
      double newValue = (_x)/(width);
      if (newValue < 0.0) newValue = 0.0;
      if (newValue > 1.0) newValue = 1.0;
    
      polygonEngine.icon(
	Icons[Selector],
	0x0004,
	(x+newValue*width-IconWidths[Selector]*height/2)*scale,
	y*scale,height*scale,height*scale);

      if (mouseDown) {
        *value = newValue;
      
        if (callback)
	  callback(*value);
      }

      if (mouseOver)
        _hotKey = (newValue < *value ? leftKey : rightKey);
    }

    return 0;
  }

  void handleKey(char key, int &action) {
    if (key == leftKey || key == rightKey) {
      if (key == leftKey) {
        if (*value == 0.0) return;
        *value -= 0.05;
        if (*value < 0.0) *value = 0.0;
      } else {
        if (*value == 1.0) return;
        *value += 0.05;
        if (*value > 1.0) *value = 1.0;
      }

      if (callback)
        callback(*value);
    }
  }
};

//Dodgy, uses some globals
struct TrackSelector : public UIObject {
  int position;

  TrackSelector(int _am,int _vm,double _x,double _y,
         double _width,double _height) {
    activeMask = _am; visibleMask = _vm;
    x = _x; y = _y; width = _width; height = _height;
  }

  int go(bool mouseDown,bool mouseClick,bool mouseOver,
          double _x, double _y, double scale, char &_hotKey, int &action) {
     int count = cdGetTrackCount();
     if (count == 0) return 0;

     position = -1;
     if (mouseOver) {
       position = int(_x/width*count)+1;
     
       if (mouseClick)
         action = -position;

       if (position < 10)
         _hotKey = '0' + position;
       else if (position == 10)
         _hotKey = '0';
     }
     
     for(int i=0;i<count;i++)
       polygonEngine.icon( 
         Icons[TrackSelect],
	 (i+1==position?0x0003:(i+1==track?0x0202:0x0100)),
	 x*scale+i*width*scale/count,
	 y*scale,
	 width*scale/count/IconWidths[TrackSelect],
	 height*scale);

     return 0;
  }

  void handleKey(char key, int &action) {
    if (key >= '1' && key <= '9')
      action = -(key-'0');
    else if (key == '0')
      action = -10;
  }
};

static UIObject *uiObjects; 
static Button *stateButton, *starsButton, *waveButton, *flameButton,
              *starButton, *diamondButton;
static TrackSelector *trackSelector;
static int mouseButtons;

void setupPalette(double dummy=0.0) {
  #define BOUND(x) ((x) > 255 ? 255 : (x))
  #define PEAKIFY(x) int(BOUND((x) - (x)*(255-(x))/255/2))
  #define MAX(x,y) ((x) > (y) ? (x) : (y))
  int i;
  unsigned char palette[768];

  double scale, fgRed, fgGreen, fgBlue, bgRed, bgGreen, bgBlue;
  fgRed = fgRedSlider;
  fgGreen = fgGreenSlider;
  fgBlue = 1.0 - MAX(fgRedSlider,fgGreenSlider);
  //scale = MAX(MAX(fgRed,fgGreen),fgBlue);
  scale = (fgRed+fgGreen+fgBlue)/2.0;
  fgRed /= scale;
  fgGreen /= scale;
  fgBlue /= scale;
  
  bgRed = bgRedSlider;
  bgGreen = bgGreenSlider;
  bgBlue = 1.0 - MAX(bgRedSlider,bgGreenSlider);
  //scale = MAX(MAX(bgRed,bgGreen),bgBlue);
  scale = (bgRed+bgGreen+bgBlue)/2.0;
  bgRed /= scale;
  bgGreen /= scale;
  bgBlue /= scale;
  
  for(i=0;i<256;i++) {
    int f = i&15, b = i/16;
    //palette[i*3+0] = PEAKIFY(b*bgRed*16+f*fgRed*16);
    //palette[i*3+1] = PEAKIFY(b*bgGreen*16+f*fgGreen*16);
    //palette[i*3+2] = PEAKIFY(b*bgBlue*16+f*fgBlue*16);

    double red = b*bgRed*16+f*fgRed*16;
    double green = b*bgGreen*16+f*fgGreen*16;
    double blue = b*bgBlue*16+f*fgBlue*16;

    double excess = 0.0;
    for(int j=0;j<5;j++) {
      red += excess / 3;
      green += excess / 3;
      blue += excess / 3;
      excess = 0.0;
      if (red > 255) { excess += red-255; red = 255; }
      if (green > 255) { excess += green-255; green = 255; }
      if (blue > 255) { excess += blue-255; blue = 255; }
    }

    double scale = (0.5 + (red+green+blue)/768.0) / 1.5;
    red *= scale;
    green *= scale;
    blue *= scale;
    
    palette[i*3+0] = BOUND(int(red));
    palette[i*3+1] = BOUND(int(green));
    palette[i*3+2] = BOUND(int(blue));
  }
  screen->setPalette(palette);
}

void setTrackProgress(double progress) {
  int interval = cdGetTrackFrame(track+1)-cdGetTrackFrame(track);
  if (interval <= 0) return;
  cdPlay(cdGetTrackFrame(track)+int(interval*progress));
}

//Visible mask
#define ALL 1
#define BUTTONBAR 2
#define TRACKBAR 4
#define DIALBAR 8
#define VOLUMEBAR 16

//Active mask
//#define ALL 1
#define PLAYING 2
#define PAUSED 4
#define STOPPED 8
#define NOCD 32 
#define OPEN 64 

static int visibleMask, cdCheckCountDown;
static int mouseX, mouseY, lastX, lastY, countDown;

void addUI(UIObject *obj) {
  obj->next = uiObjects;
  uiObjects = obj;
}

#define IconSize 0.2
#define SliderSize 0.125
void interfaceInit() {
  double x,y;
  uiObjects = 0;
  //addUI(new Button(ALL,0.025,0.525,IconSize, 0, 'x'));
  addUI(new PopperUpper(ALL,ALL,0,0,0.25,0.25, BUTTONBAR));
  addUI(stateButton = new Button(ALL,ALL,0.05,0.025,IconSize, 0, 0, true, false));
  
  addUI(new PopperUpper(ALL,BUTTONBAR,x=0.25,y=0,1.375,0.25, BUTTONBAR));
  x += 0.1; y += 0.025;
  addUI(new Button(PLAYING|PAUSED,BUTTONBAR,x,y,IconSize, SkipBack, '['));
  addUI(new Button(PAUSED|STOPPED|OPEN,BUTTONBAR,x += IconSize,y,IconSize, Play, 'p'));
  addUI(new Button(PLAYING,BUTTONBAR,x,y,IconSize, Pause, 'p'));
  addUI(new Button(PLAYING|PAUSED|OPEN,BUTTONBAR,x += IconSize,y,IconSize, Stop, 's'));
  addUI(new Button(PLAYING|PAUSED,BUTTONBAR,x += IconSize,y,IconSize, SkipFwd, ']'));
  addUI(new Button(PLAYING|PAUSED|STOPPED|NOCD, BUTTONBAR,
                   x += IconSize,y,IconSize, Open, 'e'));
  addUI(new Button(ALL, BUTTONBAR,x += IconSize,y,IconSize, Exit, 'q'));
  
  addUI(new PopperUpper(PLAYING|PAUSED|STOPPED, ALL,0,0.25,0.25,0.25, TRACKBAR));
  addUI(new PopperUpper(PLAYING|PAUSED|STOPPED, TRACKBAR,x=0.25,y=0.25,1.0,0.625, TRACKBAR));
  x += 0.1; y += 0.1;
  addUI(trackSelector = new TrackSelector(PLAYING|PAUSED|STOPPED, TRACKBAR,x,y,0.75,0.25));
  addUI(new SliderBar(PLAYING|PAUSED, TRACKBAR,x,y+=0.25,0.75,0.25,
                      &trackProgress,setTrackProgress,'{','}'));

  addUI(new PopperUpper(ALL,ALL,0,0.5,0.25,0.25, DIALBAR));
  addUI(new Button(ALL,ALL,0.05,0.525,IconSize, Bulb, 0, true, false));

  addUI(new PopperUpper(ALL,DIALBAR,x=0.25,y=0.0,1.25,1.0, DIALBAR));
  x += 0.05; y += 0.025;
  
  addUI(starsButton = new Button(ALL,DIALBAR,x,y,IconSize, Stars, 'd'));
  addUI(waveButton = new Button(ALL,DIALBAR,x+IconSize,y,IconSize, Wave, 'f'));
  addUI(flameButton = new Button(ALL,DIALBAR,x+IconSize*2.5,y,IconSize, Flame, 'g'));
  
  addUI(starButton = new Button(ALL,DIALBAR,x+IconSize*3.5,y,IconSize, Star, 'h'));
  addUI(diamondButton = new Button(ALL,DIALBAR,x+IconSize*4.5,y,IconSize, Diamond, 'j'));

  addUI(new Button(ALL,DIALBAR,x+0.4,y+0.8,IconSize, Save, '?'));
  addUI(new Button(ALL,DIALBAR,x+0.65,y+0.8,IconSize, Reset, '/'));

  y += IconSize*1.3;
  
  addUI(new Button(ALL,DIALBAR,x,y-0.05,IconSize, Bulb, 0, true));
  addUI(new SliderBar(ALL,DIALBAR,
    x+IconSize,y, 0.75, SliderSize, &brightnessTwiddler, 0, 'z', 'x'));
  
  addUI(new Button(ALL,DIALBAR,x,y+SliderSize*1,IconSize, Size, 'x', true));
  addUI(new SliderBar(ALL,DIALBAR,
    x+IconSize,y+SliderSize, 0.75, SliderSize, &starSize, setStarSize, 'c','v'));

  addUI(new Button(ALL,DIALBAR,x+0.5,y+SliderSize*2-0.025,IconSize, FgColor, 0, true));
  addUI(new SliderBar(ALL,DIALBAR,
    x,y+SliderSize*2, 0.45, SliderSize, &fgRedSlider, setupPalette, 'b','n'));
  addUI(new SliderBar(ALL,DIALBAR,
    x+0.5+SliderSize,y+SliderSize*2, 0.45, SliderSize, &fgGreenSlider, setupPalette, 'm',','));

  
  addUI(new Button(ALL,DIALBAR,x+0.5,y+SliderSize*3,IconSize, BgColor, 0, true));
  addUI(new SliderBar(ALL,DIALBAR,
    x,y+SliderSize*3, 0.45, SliderSize, &bgRedSlider, setupPalette, 'B','N'));
  addUI(new SliderBar(ALL,DIALBAR,
    x+0.5+SliderSize,y+SliderSize*3, 0.45, SliderSize, &bgGreenSlider, setupPalette, 'M','<'));
  
  addUI(new PopperUpper(ALL,ALL,0,0.75,0.25,0.25, VOLUMEBAR));
  addUI(new Button(ALL,ALL,0.05,0.775,IconSize, Speaker, 0, true, false));
  
  addUI(new PopperUpper(ALL,VOLUMEBAR,x=0.25,y=0.75,1.0,0.25, VOLUMEBAR));
  x += 0.1;// y += 0.0625;
  addUI(new SliderBar(ALL,VOLUMEBAR,x,y, 0.75, 0.25, &volume, setVolume, '-','+'));
  //static double value = 0.5;
  //addUI(new SliderBar(ALL,0,0.75,1.0,0.25,&value)); 

  //addUI(new Button(BUTTONBAR,x,y,IconSize, 1, 'x'));
  //addUI(new Button(BUTTONBAR,x += IconSize,y,IconSize, 2, 'x'));
  //addUI(new Button(BUTTONBAR,x += IconSize,y,IconSize, 3, 'x'));

  visibleMask = ALL;
  cdCheckCountDown = 0;
  mouseX = -1;
  mouseY = -1;
  lastY = -1;
  lastY = -1;
  countDown = 0;
  mouseButtons = 0;

  interfaceSyncToState();
}

void interfaceSyncToState() {
  starsButton->bright = (fadeMode == Stars); 
  flameButton->bright = (fadeMode == Flame); 
  waveButton->bright = (fadeMode == Wave);

  starButton->bright = !pointsAreDiamonds;
  diamondButton->bright = pointsAreDiamonds;

  setupPalette();
}

int changeState(int transitionSymbol) {
  if (transitionSymbol < 0) {
    cdPlay(cdGetTrackFrame(-transitionSymbol));
    return 0;
  }
  
  int retVal = 0;
  switch(transitionSymbol) {
    case Play  :
      if (state == Open) {
        cdCloseTray();
        state = NoCD;
        cdGetStatus(track, frames, state);
      } 
      if (state == Pause) cdResume(); else cdPlay(cdGetTrackFrame(1)); 
      break;
    case Pause : 
      cdPause(); break;
    case Stop  : 
      if (state == Open) {
        cdCloseTray();
        state = NoCD;
      } 
      cdStop(); 
      break;
    case Open  : 
      cdEject(); 
      state = Open;
      break;
    case SkipBack :
      cdPlay(cdGetTrackFrame(track-1));
      break;
    case SkipFwd :
      cdPlay(cdGetTrackFrame(track+1));
      break;

    case Flame :
      starsButton->bright = false;
      flameButton->bright = true;
      waveButton->bright = false;
      fadeMode = Flame;
      setStarSize(starSize);
      break;
    case Wave :
      starsButton->bright = false;
      flameButton->bright = false;
      waveButton->bright = true;
      fadeMode = Wave;
      setStarSize(starSize);
      break;
    case Stars :
      starsButton->bright = true;
      flameButton->bright = false;
      waveButton->bright = false;
      fadeMode = Stars;
      setStarSize(starSize);
      break;

    case Star :
      pointsAreDiamonds = false;
      starButton->bright = true;
      diamondButton->bright = false;
      break;
    case Diamond :
      pointsAreDiamonds = true;
      starButton->bright = false;
      diamondButton->bright = true;
      break;

    case Save :
      saveConfig();
      break;
    case Reset :
      setStateToDefaults();
      interfaceSyncToState();
      setStarSize(starSize);
      break;

    case Exit  : 
      retVal = 1; break;
  }
  return retVal;
}

bool interfaceGo() {
  int newVisibleMask = ALL;
  char keyHit;
  bool quit = false;
  int action = NotASymbol;
  int oldButtons = mouseButtons;
  
  screen->inputUpdate(mouseX,mouseY,mouseButtons,keyHit); //may also resize output

  bool mouseClick = (mouseButtons && !oldButtons);

  if ((mouseX != lastX || mouseY != lastY) && 
      lastX > 0 && lastY > 0 && 
      lastX < outWidth && lastY < outHeight)
    countDown = 40;
    
  int activeMask = ALL;
  switch(state) {
    case Play : activeMask |= PLAYING; break;
    case Pause : activeMask |= PAUSED; break;
    case Stop : activeMask |= STOPPED; break;
    case NoCD : activeMask |= NOCD; break;
    case Open : activeMask |= OPEN; break;
    default : break;
  }

  if (countDown) {
    countDown--;

    double scale = 
      (outWidth*0.625 < outHeight ? outWidth*0.625 : outHeight);
    double scaledX = mouseX / scale;
    double scaledY = mouseY / scale;

    char hotKey = 0;

    polygonEngine.clear();

    stateButton->icon = state;


    for(UIObject *i=uiObjects;i;i = i->next) {
      if ((i->visibleMask & visibleMask) && (i->activeMask & activeMask))
	newVisibleMask |= i->go(mouseButtons,mouseClick,
	    (scaledX >= i->x &&
	     scaledY >= i->y &&
	     scaledX < i->x+i->width &&
	     scaledY < i->y+i->height),
	    scaledX - i->x,
	    scaledY - i->y,
	    scale,
	    hotKey,
	    action);
    }

    if (activeMask & (PLAYING|PAUSED|STOPPED)) {
      int trackNumber = (activeMask&(PLAYING|PAUSED) ? track : -1);
      unsigned short trackColor = 0x0100;
      if ((visibleMask&TRACKBAR) && trackSelector->position != -1) {
	trackNumber = trackSelector->position;
	trackColor = 0x0003;
      }

      if (trackNumber != -1) {
	if (trackNumber > 9)
	  polygonEngine.icon(Icons[Zero+trackNumber/10%10],trackColor,
	     scale*0.03125,scale*0.25,scale*0.25,scale*0.25);
	
	polygonEngine.icon(Icons[Zero+trackNumber%10],trackColor,
	   scale*0.125,scale*0.25,scale*0.25,scale*0.25);
      }
    }

    visibleMask = newVisibleMask;  
    if (visibleMask != 1)
      countDown = 20;

    polygonEngine.icon(Icons[Pointer],0x0303,mouseX,mouseY,50,50);

    polygonEngine.apply(outputBmp.data);

    char hint[2] = " ";
    hint[0] = hotKey;
    putString(hint,mouseX+6,mouseY+7,0,0);
  }
    
  if (keyHit)
    for(UIObject *i=uiObjects;i;i = i->next) 
      if (i->activeMask & activeMask)
        i->handleKey(keyHit,action);


  lastX = mouseX;
  lastY = mouseY;

  if (action != NotASymbol)
    quit = quit || changeState(action);

  if (state != Plug && (action != NotASymbol || cdCheckCountDown < 0)) {
    SymbolID oldState = state;
    cdGetStatus(track, frames, state);
    if (action == NotASymbol &&
        (oldState == Play || oldState == Open || oldState == NoCD) &&
	state == Stop) {
      if (playListLength == 0)
        cdPlay(cdGetTrackFrame(1));
      else {
        int track = atoi(playList[playListPosition]);
	playListPosition = (playListPosition+1) % playListLength;
	cdPlay(cdGetTrackFrame(track),cdGetTrackFrame(track+1));
      }
    }

    int interval = cdGetTrackFrame(track+1)-cdGetTrackFrame(track);
    if (interval > 0)
      trackProgress = double(frames)/interval;
    else
      trackProgress = 0.0;
    
    cdCheckCountDown = 100;
  } else
    cdCheckCountDown -= (countDown ? 10 : 1);

  if (mouseClick && action == NotASymbol && !(visibleMask&~ALL)) {
    screen->toggleFullScreen();
    screen->inputUpdate(mouseX,mouseY,mouseButtons,keyHit);
    lastX = mouseX;
    lastY = mouseY;
  }

  return quit;
}

void interfaceEnd() {
  while(uiObjects) {
    UIObject *next = uiObjects->next;
    delete uiObjects;
    uiObjects = next;
  }
}
