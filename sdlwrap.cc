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
#include <stdint.h>
#include <time.h>
#include <string.h>
#include "syna.h"
#include <SDL.h>
#if defined(EMSCRIPTEN) && !SDL_VERSION_ATLEAST(2,0,0)
#include <emscripten.h>
#endif

#ifdef HAVE_ICON
extern unsigned char syn_icon_rgb[];
#endif

static SDL_Surface *surface;
static SDL_Color sdlPalette[256];
#if SDL_VERSION_ATLEAST(2,0,0)
static SDL_Window *window;
static SDL_Palette sdl2Palette = { 256, sdlPalette };
#endif
static bool fullscreen;
static int scaling; //currently only supports 1, 2
static int depth; // bits per pixel
uint32_t *colorlookup; // pallette for 32bpp

static void sdlError(const char *str) {
  fprintf(stderr, "synaesthesia: Error %s\n", str);
  fprintf(stderr,"(reason for error: %s)\n", SDL_GetError());
  exit(1);
}

#if SDL_VERSION_ATLEAST(2,0,0)
static void createSurface() {
  surface = SDL_GetWindowSurface(window);
  if (surface == NULL) sdlError("at SDL_GetWindowSurface");

  scaling = 1;
}
#else
static void createSurface() {
  Uint32 videoflags = SDL_HWSURFACE | SDL_DOUBLEBUF |
#if defined(EMSCRIPTEN) && !SDL_VERSION_ATLEAST(2,0,0)
                      /* This enables more optimized 8 bpp */
                      ((depth == 8) ? SDL_HWPALETTE : 0) |
#endif
                      SDL_RESIZABLE | (fullscreen?SDL_FULLSCREEN:0);


  /* Get available fullscreen modes */
  SDL_Rect **modes = SDL_ListModes(0, videoflags);

  bool any = false;

  if(modes == (SDL_Rect **)-1){
    /* All resolutions ok */
    any = true;
  } else {
    /* Is there a resolution that will fit the display nicely */
    for(int i=0;modes[i];i++)
      if (modes[i]->w < outWidth*2 || modes[i]->h < outHeight*2)
        any = true;
  }

  scaling = (any?1:2); 
               
  surface = SDL_SetVideoMode(outWidth*scaling, outHeight*scaling,
                             depth, videoflags);

  if (!surface)
    sdlError("setting video mode");

  if (depth == 8)
    SDL_SetColors(surface, sdlPalette, 0, 256);
}
#endif

void SdlScreen::setPalette(unsigned char *palette) {
  if (depth == 32) {
    colorlookup = (uint32_t*)palette;
    return;
  }

  for(int i=0;i<256;i++) {
    sdlPalette[i].r = palette[i*3+0];
    sdlPalette[i].g = palette[i*3+1];
    sdlPalette[i].b = palette[i*3+2];
  }
#if SDL_VERSION_ATLEAST(2,0,0)
  SDL_SetSurfacePalette(surface, &sdl2Palette);
#else
  SDL_SetColors(surface, sdlPalette, 0, 256);
#endif
}

bool SdlScreen::init(int xHint,int yHint,int width,int height,bool fullscreen,
                     int bpp)
{
  outWidth = width;
  outHeight = height;
  ::fullscreen = fullscreen;
  depth = bpp;

  if (depth != 8 && depth != 32)
    return false;

  /* Initialize SDL */
  if ( SDL_Init(SDL_INIT_VIDEO) < 0 )
    sdlError("initializing SDL");

#ifdef HAVE_ICON
  SDL_Surface *iconsurface =
    SDL_CreateRGBSurfaceFrom(syn_icon_rgb, 64, 64, 24, 64*3,
#ifdef LITTLEENDIAN
                             0x0000ff, 0x00ff00, 0xff0000, 0
#else
                             0xff0000, 0x00ff00, 0x0000ff, 0
#endif
                             );
#endif /* HAVE_ICON */

#if SDL_VERSION_ATLEAST(2,0,0)
  window = SDL_CreateWindow("Synaesthesia",
                            fullscreen ? 0 : xHint,
                            fullscreen ? 0 : yHint,
                            outWidth, outHeight,
                            (fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP :
                             SDL_WINDOW_RESIZABLE) | SDL_WINDOW_SHOWN);
  if (window == NULL) sdlError("at SDL_CreateWindow");
#else
#ifdef EMSCRIPTEN
  /* Optimize SDL 1 performance */
  EM_ASM(
    SDL.defaults.copyOnLock = false;
    SDL.defaults.discardOnLock = true;
    SDL.defaults.opaqueFrontBuffer = false;
  );
#endif
#endif

#ifdef HAVE_ICON
#if SDL_VERSION_ATLEAST(2,0,0)
  SDL_SetWindowIcon(window, iconsurface);
#else
  /* Must be called before SDL_SetVideoMode() */
  SDL_WM_SetIcon(iconsurface, NULL);
#endif
  SDL_FreeSurface(iconsurface);
#endif /* HAVE_ICON */

  createSurface();

// FIXME: There is no keyboard input with SDL2
#if !SDL_VERSION_ATLEAST(2,0,0)
  SDL_EnableUNICODE(1);
#endif
  SDL_ShowCursor(0);

  return true;
}

void SdlScreen::end(void) {
  SDL_Quit();
}

void SdlScreen::toggleFullScreen(void) {
  fullscreen = !fullscreen;
  createSurface();
}

void SdlScreen::inputUpdate(int &mouseX,int &mouseY,int &mouseButtons,char &keyHit) {    
  SDL_Event event;

  keyHit = 0;
  
  while ( SDL_PollEvent(&event) > 0 ) {
    switch (event.type) {
      case SDL_MOUSEBUTTONUP:
      case SDL_MOUSEBUTTONDOWN:
        if ( event.button.state == SDL_PRESSED ) {
#if SDL_VERSION_ATLEAST(2,0,2)
          if (event.button.clicks == 2) {
              mouseButtons |= SYN_DBL_CLICK;
          }
#else
          // Earlier SDL versions don't report double clicks
          static Uint32 dblclicktm = 0;
          Uint32 clicktime = SDL_GetTicks();
          // Like !SDL_TICKS_PASSED(), which may not be available
          if ((Sint32)(dblclicktm - clicktime) > 0) {
            mouseButtons |= SYN_DBL_CLICK;
          }
          dblclicktm = clicktime + 200;
#endif // !SDL_VERSION_ATLEAST(2,0,2)
          mouseButtons |= 1 << event.button.button;
        } else {
          mouseButtons &= ~( 1 << event.button.button );
          mouseButtons &= ~SYN_DBL_CLICK;
        }
        mouseX = event.button.x / scaling;
        mouseY = event.button.y / scaling;
	break;
      case SDL_MOUSEMOTION :
        mouseX = event.motion.x / scaling;
        mouseY = event.motion.y / scaling;
	break;
#if SDL_VERSION_ATLEAST(2,0,0)
      case SDL_WINDOWEVENT:
        switch (event.window.event) {
        case SDL_WINDOWEVENT_LEAVE:
          /* Lost focus, hide mouse */
          mouseX = outWidth;
          mouseY = outHeight;
          break;
        case SDL_WINDOWEVENT_RESIZED:
          allocOutput(event.window.data1, event.window.data2);
          createSurface();
          break;
        }
        break;
#else
      case SDL_ACTIVEEVENT :
        /* Lost focus, hide mouse */
        if (!event.active.gain) {
          mouseX = outWidth;
          mouseY = outHeight;
        }
      case SDL_KEYDOWN:
        ///* Ignore key releases */
        //if ( event.key.state == SDL_RELEASED ) {
        //  break;
        //}
        /* Ignore ALT-TAB for windows */
        if ( (event.key.keysym.sym == SDLK_LALT) ||
             (event.key.keysym.sym == SDLK_TAB) ) {
          break;
        }

	if (event.key.keysym.unicode > 255)
	  break;

	keyHit = event.key.keysym.unicode;
	return;
      case SDL_VIDEORESIZE:
        event.resize.w &= ~3;
        allocOutput(event.resize.w,event.resize.h);
        createSurface();
        break;
#endif
      case SDL_QUIT:
        keyHit = 'q';        
        return;
      default:
        break;
    }
  }
}

void SdlScreen::show(void) { 
  attempt(SDL_LockSurface(surface),"locking screen for output.");

  if (depth == 32) {
    if (scaling == 1) {
      uint16_t *in = (uint16_t*)output;
      uint8_t *line = (uint8_t*)(surface->pixels);
      for (int y = 0; y < outHeight; y++) {
        uint32_t *out = (uint32_t*)line;
        for (int x = 0; x < outWidth; x++) {
          *(out++) = colorlookup[*(in++)];
        }
        line += surface->pitch;
      }
    } else {
      // scaling 2
      uint16_t *in = (uint16_t*)output;
      uint8_t *line = (uint8_t*)(surface->pixels);
      for(int y = 0; y < outHeight; y++) {
        uint32_t *lp1 = (uint32_t*)line;
        line += surface->pitch;
        uint32_t *lp2 = (uint32_t*)line;
        line += surface->pitch;
        for(int x = 0; x < outWidth; x++) {
          register uint32_t v = colorlookup[*(in++)];
          *(lp1++) = v; *(lp1++) = v;
          *(lp2++) = v; *(lp2++) = v;
        } // for each pixel in line
      } // for each pair of lines
    } // scaling 2
  } else
  if (scaling == 1) {
    register uint32_t *ptr2 = (uint32_t*)output;
    int lines, linelen;
    if (surface->pitch == outWidth) {
      // Do everything at once
      lines = 1;
      linelen = outWidth*outHeight/sizeof(*ptr2);
    } else {
      // Do one line at once, adjusting for pitch between lines
      lines = outHeight;
      linelen = outWidth/sizeof(*ptr2);
    }

    for (int y = 0; y < lines; y++) {
      uint32_t *ptr1 = (uint32_t*)( (uint8_t*)surface->pixels +
                                    surface->pitch * y);
      int i = linelen;

      do {
        // Asger Alstrup Nielsen's (alstrup@diku.dk)
        // optimized 32 bit screen loop
        register unsigned int const r1 = *(ptr2++);
        register unsigned int const r2 = *(ptr2++);

  #ifdef LITTLEENDIAN
        register unsigned int const v = 
            ((r1 & 0x000000f0ul) >> 4)
          | ((r1 & 0x0000f000ul) >> 8)
          | ((r1 & 0x00f00000ul) >> 12)
          | ((r1 & 0xf0000000ul) >> 16);
        *(ptr1++) = v | 
          ( ((r2 & 0x000000f0ul) << 12)
          | ((r2 & 0x0000f000ul) << 8)
          | ((r2 & 0x00f00000ul) << 4)
          | ((r2 & 0xf0000000ul)));
  #else
        register unsigned int const v = 
            ((r2 & 0x000000f0ul) >> 4)
          | ((r2 & 0x0000f000ul) >> 8)
          | ((r2 & 0x00f00000ul) >> 12)
          | ((r2 & 0xf0000000ul) >> 16);
        *(ptr1++) = v | 
          ( ((r1 & 0x000000f0ul) << 12)
          | ((r1 & 0x0000f000ul) << 8)
          | ((r1 & 0x00f00000ul) << 4)
          | ((r1 & 0xf0000000ul)));
  #endif
      } while (--i);
    }
  } else {
    // SDL has no standard image scaling routine (!)
    uint8_t *pixels = (uint8_t*)(surface->pixels);
    for(int y=0;y<outHeight;y++) {
      uint32_t *p1 = (uint32_t*)(output+y*outWidth*2);
      uint32_t *p2 = (uint32_t*)(pixels+y*2*surface->pitch);
      uint32_t *p3 = (uint32_t*)(pixels+(y*2+1)*surface->pitch);
      for(int x=0;x<outWidth;x+=2) {
        uint32_t v = *(p1++);
        v = ((v&0x000000f0) >> 4)
          | ((v&0x0000f000) >> 8)
          | ((v&0x00f00000) >> 4)
          | ((v&0xf0000000) >> 8);
        v |= v<<8;
        *(p2++) = v;
        *(p3++) = v;
      }
    }
  }

  SDL_UnlockSurface(surface);

  //SDL_UpdateRect(surface, 0, 0, 0, 0);
#if SDL_VERSION_ATLEAST(2,0,0)
  SDL_UpdateWindowSurface(window);
#else
  SDL_Flip(surface);
#endif
}

int SdlScreen::getDepth() {
  return depth;
}

/* Calculate right shift needed to align colour component MSB with bit 7.
 * This assumes a 32 bpp or 24 bpp mode, where a left shift shouldn't
 * ever be necessary.
 *
 * SDL does provide Rshift, Bshift and Gshift, but those are documented as
 * "(internal use)" and Emscripten's SDL doesn't set them.
 */
static int getPixelShift(Uint32 mask) {
  int shift = 0;
  while ((mask & ~0xFF) != 0) {
      shift++;
      mask >>= 1;
  }
  return shift;
}

void SdlScreen::getPixelFormat(int *rshift, unsigned long *rmask,
                               int *gshift, unsigned long *gmask,
                               int *bshift, unsigned long *bmask,
                               int *ashift, unsigned long *amask) {
  SDL_PixelFormat *fmt = surface->format;
  *rshift = getPixelShift(fmt->Rmask);
  *rmask = fmt->Rmask;
  *gshift = getPixelShift(fmt->Gmask);
  *gmask = fmt->Gmask;
  *bshift = getPixelShift(fmt->Bmask);
  *bmask = fmt->Bmask;
  *ashift = getPixelShift(fmt->Amask);
  *amask = fmt->Amask;
}
