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

#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <stdint.h>
#include "syna.h"

#if HAVE_LIBVGA

#include <vga.h>
#include <vgamouse.h>

static unsigned char *scr;
static int keyboardDevice;

static void setOnePalette(int i,int r,int g,int b) {
  vga_setpalette(i,r/4,g/4,b/4);
}

bool SvgaScreen::init(int xHint,int yHint,int widthHint,int heightHint,bool fullscreen) {
  attempt(vga_init(),"initializing svgalib");
  if (!vga_hasmode(G320x200x256)) 
    error("requesting 320x200 graphics mode");
  
  attempt(vga_setmode(G320x200x256),"entering 320x200 graphics mode");

  scr = vga_getgraphmem();
  outWidth = 320;
  outHeight = 200;
 
  attemptNoDie(mouse_init("/dev/mouse",
    vga_getmousetype(),MOUSE_DEFAULTSAMPLERATE),"initializing mouse");
  mouse_setxrange(-1,321);
  mouse_setyrange(-1,201);
  mouse_setposition(160,34);
  //mouse_setscale(32);

  /*#define BOUND(x) ((x) > 255 ? 255 : (x))
  #define PEAKIFY(x) BOUND((x) - (x)*(255-(x))/255/2)
  int i;
  for(i=0;i<256;i++)
    setPalette(i,PEAKIFY((i&15*16)),
                 PEAKIFY((i&15)*16+(i&15*16)/4),
                 PEAKIFY((i&15)*16));
  */

  /* Get keyboard input descriptor */
  if (!isatty(0)) {
    char *tty;

    //Ok, we're getting piped input, so we can't use stdin.
    //Find out where stdout is going and read from it.
    tty = ttyname(1);

    keyboardDevice = -1;

    if (tty) {
      keyboardDevice = open(tty, O_RDONLY);

      if (keyboardDevice != -1) {
        termios term;
        tcgetattr(keyboardDevice, &term);
        cfmakeraw(&term);
        tcsetattr(keyboardDevice, TCSANOW, &term);
      }
    }
  } else 
    keyboardDevice = 0;  
  
  if (keyboardDevice != -1)
    fcntl(keyboardDevice,F_SETFL,O_NONBLOCK);

  return true;
}

void SvgaScreen::setPalette(unsigned char *palette) {
  int i;
  for(i=0;i<256;i++)
    setOnePalette(i,palette[i*3],palette[i*3+1],palette[i*3+2]);
}

void SvgaScreen::end() {
  if (keyboardDevice > 0)
    close(keyboardDevice);
  
  mouse_close();
  vga_setmode(TEXT);
}

void SvgaScreen::inputUpdate(int &mouseX,int &mouseY,int &mouseButtons,char &keyHit) {
  mouse_update(); 
  mouseX = mouse_getx();
  mouseY = mouse_gety();
  mouseButtons = mouse_getbutton();
  
  if (keyboardDevice == -1 || read(keyboardDevice, &keyHit, 1) != 1)
    keyHit = 0;
}

void SvgaScreen::show(void) {
  register uint32_t *ptr2 = (uint32_t*)output;
  uint32_t *ptr1 = (uint32_t*)scr;
  int i = 320*200/sizeof(uint32_t);
  // Asger Alstrup Nielsen's (alstrup@diku.dk)
  // optimized 32 bit screen loop
  do {
    //Original bytewize version:
    //unsigned char v = (*(ptr2++)&15*16);
    //*(ptr1++) = v|(*(ptr2++)>>4);
    register uint32_t const r1 = *(ptr2++);
    register uint32_t const r2 = *(ptr2++);

    //Fade will continue even after value > 16
    //thus black pixel will be written when values just > 0
    //thus no need to write true black
    //if (r1 || r2) {
#ifdef LITTLEENDIAN
      register uint32_t const v = 
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
      register uint32_t const v = 
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
    //} else {
    //  ptr1++;
    //}  
  } while (--i); 
}

#endif
