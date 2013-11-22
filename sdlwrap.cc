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

#if HAVE_SDL

#include "SDL.h"

static SDL_Surface *surface;
static SDL_Color sdlPalette[256];
static bool fullscreen;
static int scaling; //currently only supports 1, 2

static void createSurface() {
  Uint32 videoflags = SDL_HWSURFACE | SDL_DOUBLEBUF |
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
               
  surface = SDL_SetVideoMode(outWidth*scaling, outHeight*scaling, 8, videoflags);
  SDL_SetColors(surface, sdlPalette, 0, 256);

  if (!surface)
    error("setting video mode");
}
  
void SdlScreen::setPalette(unsigned char *palette) {
  for(int i=0;i<256;i++) {
    sdlPalette[i].r = palette[i*3+0];
    sdlPalette[i].g = palette[i*3+1];
    sdlPalette[i].b = palette[i*3+2];
  }
  
  SDL_SetColors(surface, sdlPalette, 0, 256);
}

bool SdlScreen::init(int xHint,int yHint,int width,int height,bool fullscreen) 
{
  Uint32 videoflags;

  outWidth = width;
  outHeight = height;
  ::fullscreen = fullscreen;

  /* Initialize SDL */
  if ( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
    char str[1000];
    printf(str, "Could not initialize SDL library: %s\n",SDL_GetError());
    return false;
  }

  SDL_WM_SetCaption("Synaesthesia","synaesthesia");

  createSurface();

  SDL_EnableUNICODE(1);
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
  bool resized = false;
 
  keyHit = 0;
  
  while ( SDL_PollEvent(&event) > 0 ) {
    switch (event.type) {
      case SDL_MOUSEBUTTONUP:
      case SDL_MOUSEBUTTONDOWN:
        if ( event.button.state == SDL_PRESSED ) 
          mouseButtons |= 1 << event.button.button;
        else	
          mouseButtons &= ~( 1 << event.button.button );
        mouseX = event.button.x / scaling;
        mouseY = event.button.y / scaling;
	break;
      case SDL_MOUSEMOTION :
        mouseX = event.motion.x / scaling;
        mouseY = event.motion.y / scaling;
	break;
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

  if (scaling == 1) {
    register uint32_t *ptr2 = (uint32_t*)output;
    uint32_t *ptr1 = (uint32_t*)( surface->pixels );
    int i = outWidth*outHeight/sizeof(*ptr2);

    do {
      // Asger Alstrup Nielsen's (alstrup@diku.dk)
      // optimized 32 bit screen loop
      register unsigned int const r1 = *(ptr2++);
      register unsigned int const r2 = *(ptr2++);
    
      //if (r1 || r2) {
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
      //} else ptr1++;
    } while (--i); 
  
  } else {
    // SDL has no standard image scaling routine (!)
    uint8_t *pixels = (uint8_t*)(surface->pixels);
    for(int y=0;y<outHeight;y++) {
      uint32_t *p1 = (uint32_t*)(output+y*outWidth*2);
      uint32_t *p2 = (uint32_t*)(pixels+y*2*outWidth*2);
      uint32_t *p3 = (uint32_t*)(pixels+(y*2+1)*outWidth*2);
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
  SDL_Flip(surface);
}

#endif
