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
#include <math.h>
#include <string.h>
#include "syna.h"

double cosTable[NumSamples], negSinTable[NumSamples];
int bitReverse[NumSamples];
/* Mapping from FFT bin to vertical position
 * as fraction of vertical height, from 1.0 to 0.0.
 */
double binToVert[NumSamples/2];
/* Scale factors for FFT bin brightness */
double binScale[NumSamples/2];
/* Hamming window for FFT */
double hamming[NumSamples];
bool useWindow;
int scaleDown[256];
int maxStarRadius;
unsigned char addPixelTab[0x10000];

int bitReverser(int i) {
  int sum=0,j;
  for(j=0;j<LogSize;j++) {
    sum = (i&1)+sum*2;
    i >>= 1;
  }
  return sum;
}

void fft(double *x,double *y) {
  int n2 = NumSamples, n1;
  int twoToTheK;
  for(twoToTheK=1;twoToTheK<NumSamples;twoToTheK*=2) {
    n1 = n2;
    n2 /= 2;
    for(int j=0;j<n2;j++) {
      double c = cosTable[j*twoToTheK&(NumSamples-1)], 
             s = negSinTable[j*twoToTheK&(NumSamples-1)];
      for(int i=j;i<NumSamples;i+=n1) {
        int l = i+n2;
        double xt = x[i] - x[l];
	x[i] = (x[i] + x[l]);
	double yt = y[i] - y[l];
	y[i] = (y[i] + y[l]);
	x[l] = xt*c - yt*s;
	y[l] = xt*s + yt*c;
      }
    }
  }
}

void coreInit(bool logfreq, bool window, bool addsq) {
  int i;

  for(i=0;i<NumSamples;i++) {
    negSinTable[i] = -sin(3.141592*2.0/NumSamples*i);
    cosTable[i] = cos(3.141592*2.0/NumSamples*i);
    bitReverse[i] = bitReverser(i);
  }

  double globalscale = 1.0;

  if (addsq) {
    for (i=0;i<0x10000;i++) {
      double pix, br, total;
      pix = i >> 8;
      br = i & 0xFF;
      total = sqrt(pix*pix+br*br);
      if (total > 255.0) {
        addPixelTab[i] = 255;
      } else {
        addPixelTab[i] = (unsigned char)(total + 0.5);
      }
    }
    /* Compensate for brightness loss */
    globalscale *= 2.7;
  } else {
    for (i=0;i<0x10000;i++) {
      unsigned char pix, br;
      pix = i >> 8;
      br = i & 0xFF;
      if (pix < 255-br) pix += br; else pix = 255;
      addPixelTab[i] = pix;
    }
  }

  /* Bin 0 is not displayed, so don't bother with it */
  if (logfreq) {
    /* The MIDI formula would devote too much of the screen to very
     * low frequencies. This fixes that problem with minimal effect
     * on higher frequencies.
     */
    static const double bass_shift = 200.0;
    for(i=1;i<NumSamples/2;i++) {
      double binfreq = ((double)i) * Frequency / NumSamples;
      /* Use MIDI tuning standard formula to map frequency */
      binToVert[i] = 69.0 + 12.0 * log2((binfreq + bass_shift)/440.0);
    }

    /* Normalize binToVert[] to 1.0 to 0.0 range.
     * (Inverted so low frequencies are displayed lower.)
     */
    double shift = binToVert[1];
    double scale = binToVert[NumSamples/2-1] - binToVert[1];
    for(i=1;i<NumSamples/2;i++) {
      binToVert[i] = 1.0 - (binToVert[i] - shift) / scale;
    }

    /* Calculate scale factors for bins. */
    for(i=1;i<NumSamples/2-1;i++) {
      binScale[i] = pow(i / (binToVert[i] - binToVert[i+1]), 0.13)
                    * 20 * globalscale;
    }
    binScale[NumSamples/2-1] = binScale[NumSamples/2-2];
  } else {
    /* Set up binToVert[] and binScale[] for results like before. */
    for(i=1;i<NumSamples/2;i++) {
      binToVert[i] = 1.0 - (double)(i-1)/(NumSamples/2-2);
      binScale[i] = i * globalscale;
    }
  }

  useWindow = window;
  if (window) {
    for (i=0;i<NumSamples;i++) {
      /* This has been scaled to maintain amplitude */
      hamming[i] = 1-0.852*cos(2*M_PI*i/(NumSamples-1));
    }
  }
}

void setStarSize(double size) {
  //int factor = (fadeMode == Flame ? 100 : 
  //              (fadeMode == Wave ? 150 : 200));

  double fadeModeFudge = (fadeMode == Wave ? 0.4 : 
                (fadeMode == Flame ? 0.6 : 0.78));

  int factor;
  if (size > 0.0)
    factor = int(exp(log(fadeModeFudge) / (size*8.0))*255);
  else
    factor = 0;

  if (factor > 255) factor = 255;

  for(int i=0;i<256;i++)
    scaleDown[i] = i*factor>>8;

  maxStarRadius = 1;
  for(int i=255;i;i = scaleDown[i])
    maxStarRadius++;
}

inline void addPixel(int x,int y,int br1,int br2) {
  if (x < 0 || x >= outWidth || y < 0 || y >= outHeight) return;
  
  unsigned char *p = output+x*2+y*outWidth*2;
  //In 2.4: if (p[0] < 255-br1) p[0] += br1; else p[0] = 255;
  //In 2.4: if (p[1] < 255-br2) p[1] += br2; else p[1] = 255;
  p[0] = addPixelTab[(p[0]<<8)|br1];
  p[1] = addPixelTab[(p[1]<<8)|br2];
  //p += lastOutput-output;
  //if (p[0] < 255-br1) p[0] += br1; else p[0] = 255;
  //if (p[1] < 255-br2) p[1] += br2; else p[1] = 255;
}

inline void addPixelFast(unsigned char *p,int br1,int br2) {
  //In 2.4: if (p[0] < 255-br1) p[0] += br1; else p[0] = 255;
  //In 2.4: if (p[1] < 255-br2) p[1] += br2; else p[1] = 255;
  p[0] = addPixelTab[(p[0]<<8)|br1];
  p[1] = addPixelTab[(p[1]<<8)|br2];
  //p += lastOutput-output;
  //if (p[0] < 255-br1) p[0] += br1; else p[0] = 255;
  //if (p[1] < 255-br2) p[1] += br2; else p[1] = 255;
}

void fadeFade() {
  /* The fade calculations were unsuitable for 32 bpp output
   * because they did not fade all the way down to 0.
   * This table matches the bytewise version below, except at the
   * low end, where each brightness is decreased by at least one.
   */
  static const uint8_t fadetab[256] = {
    0,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,
    15,16,17,18,19,20,20,21,22,23,24,25,26,27,28,29,
    29,30,31,32,33,34,35,36,37,38,39,39,40,41,42,43,
    44,45,46,47,48,49,49,50,51,52,53,54,55,56,57,58,
    58,59,60,61,62,63,64,65,66,67,68,68,69,70,71,72,
    73,74,75,76,77,78,78,79,80,81,82,83,84,85,86,87,
    87,88,89,90,91,92,93,94,95,96,97,97,98,99,100,101,
    102,103,104,105,106,107,107,108,109,110,111,112,113,114,115,116,
    116,117,118,119,120,121,122,123,124,125,126,126,127,128,129,130,
    131,132,133,134,135,136,136,137,138,139,140,141,142,143,144,145,
    145,146,147,148,149,150,151,152,153,154,155,155,156,157,158,159,
    160,161,162,163,164,165,165,166,167,168,169,170,171,172,173,174,
    174,175,176,177,178,179,180,181,182,183,184,184,185,186,187,188,
    189,190,191,192,193,194,194,195,196,197,198,199,200,201,202,203,
    203,204,205,206,207,208,209,210,211,212,213,213,214,215,216,217,
    218,219,220,221,222,223,223,224,225,226,227,228,229,230,231,232
  };
  register uint8_t *ptr = (uint8_t*)output;
  int i = outWidth*outHeight*2;
  do {
    uint8_t x = *ptr;
    *(ptr++) = fadetab[x];
    //Bytewize version was: *(ptr++) -= *ptr+(*ptr>>1)>>4;
#if 0
    //Optimized 32 bit version processing 4 bytes at a time was:
    uint32_t x = *ptr;
    if (x)
        *(ptr++) = x - ((x & 0xf0f0f0f0ul) >> 4) - ((x & 0xe0e0e0e0ul) >> 5);
    else
      ptr++;
#endif
  } while(--i > 0);
}
 
inline unsigned char getPixel(int x,int y,int where) {
  if (x < 0 || y < 0 || x >= outWidth || y >= outHeight) return 0;
  return lastOutput[where];
}

inline void fadePixelWave(int x,int y,int where,int step) {
  short j = 
    ( short(getPixel(x-1,y,where-2))+
    getPixel(x+1,y,where+2)+
    getPixel(x,y-1,where-step)+
    getPixel(x,y+1,where+step)
    >> 2)
    +lastOutput[where];
  if (!j) { output[where] = 0; return; }
  j = j
    -lastLastOutput[where]
    -1;
  if (j < 0) output[where] = 0;
  else if (j & (255*256)) output[where] = 255;
  else output[where] = j;
}

void fadeWave() {
  unsigned short *t = lastLastOutputBmp.data;
  lastLastOutputBmp.data = lastOutputBmp.data;
  lastOutputBmp.data = outputBmp.data;
  outputBmp.data = t;

  int x,y,i,j,start,end;
  int step = outWidth*2;
  for(x=0,i=0,j=outWidth*(outHeight-1)*2;x<outWidth;x++,i+=2,j+=2) {
    fadePixelWave(x,0,i,step);
    fadePixelWave(x,0,i+1,step);
    fadePixelWave(x,outHeight-1,j,step);
    fadePixelWave(x,outHeight-1,j+1,step);
  }

  for(y=1,i=outWidth*2,j=outWidth*4-2;y<outHeight;y++,i+=step,j+=step) {
    fadePixelWave(0,y,i,step);
    fadePixelWave(0,y,i+1,step);
    fadePixelWave(outWidth-1,y,j,step);
    fadePixelWave(outWidth-1,y,j+1,step);
  }

  for(y=1,
      start=outWidth*2+2,
      end=outWidth*4-2; y<outHeight-1; y++,start+=step,end+=step) {
    int i = start;
    do {
      short j = 
	( short(lastOutput[i-2])+
	lastOutput[i+2]+
	lastOutput[i-step]+
	lastOutput[i+step]
	>> 2)
	+lastOutput[i];
      if (!j) { 
        output[i] = 0; 
      } else {
        j = j
  	  -lastLastOutput[i]
	  -1;
        if (j < 0) output[i] = 0;
        else if (j & (255*256)) output[i] = 255;
        else output[i] = j; 
      }
    } while(++i < end);
  }
}

inline void fadePixelHeat(int x,int y,int where,int step) {
  short j = 
    ( short(getPixel(x-1,y,where-2))+
    getPixel(x+1,y,where+2)+
    getPixel(x,y-1,where-step)+
    getPixel(x,y+1,where+step)
    >> 2)
    +lastOutput[where];
  if (!j) { output[where] = 0; return; }
  j = j
    -lastLastOutput[where]
    -1;
  if (j < 0) output[where] = 0;
  else if (j & (255*256)) output[where] = 255;
  else output[where] = j;
}

void fadeHeat() {
  unsigned short *t = lastLastOutputBmp.data;
  lastLastOutputBmp.data = lastOutputBmp.data;
  lastOutputBmp.data = outputBmp.data;
  outputBmp.data = t;

  int x,y,i,j,start,end;
  int step = outWidth*2;
  for(x=0,i=0,j=outWidth*(outHeight-1)*2;x<outWidth;x++,i+=2,j+=2) {
    fadePixelHeat(x,0,i,step);
    fadePixelHeat(x,0,i+1,step);
    fadePixelHeat(x,outHeight-1,j,step);
    fadePixelHeat(x,outHeight-1,j+1,step);
  }

  for(y=1,i=outWidth*2,j=outWidth*4-2;y<outHeight;y++,i+=step,j+=step) {
    fadePixelHeat(0,y,i,step);
    fadePixelHeat(0,y,i+1,step);
    fadePixelHeat(outWidth-1,y,j,step);
    fadePixelHeat(outWidth-1,y,j+1,step);
  }

  for(y=1,
      start=outWidth*2+2,
      end=outWidth*4-2; y<outHeight-1; y++,start+=step,end+=step) {
    int i = start;
    do {
      short j = 
	( short(lastOutput[i-2])+
	lastOutput[i+2]+
	+lastOutput[i-step]
	+lastOutput[i+step]
	>> 2)
	+lastOutput[i];
      if (!j) { 
        output[i] = 0; 
      } else {
        j = j
  	  -lastLastOutput[i]
  	  +(lastLastOutput[i]
	   -lastOutput[i]>>2)
	  -1;
        if (j < 0) output[i] = 0;
        else if (j & (255*256)) output[i] = 255;
        else output[i] = j; 
      }
    } while(++i < end);
  }
}

void fade() { 
  switch(fadeMode) {
    case Stars :
      fadeFade(); 
      break;
    case Flame :
      fadeHeat();
      break;
    case Wave :
      fadeWave();
      break;
    default:
      break;
  }
}

int coreGo() { 
  double x[NumSamples], y[NumSamples];
  double a[NumSamples], b[NumSamples];
  double energy;
  int clarity[NumSamples]; //Surround sound
  int i,j,k;
  
  int brightFactor = int(Brightness * brightnessTwiddler /(starSize+0.01));  

  if (-1 == getNextFragment())
    return -1;

  if (useWindow) {
    for(i=0;i<NumSamples;i++) {
      x[i] = data[i*2] * hamming[i];
      y[i] = data[i*2+1] * hamming[i];
    }
  } else {
    for(i=0;i<NumSamples;i++) {
      x[i] = data[i*2];
      y[i] = data[i*2+1];
    }
  }

  fft(x,y);

  energy = 0.0;

  for(i=0 +1;i<NumSamples/2;i++) {
    double x1 = x[bitReverse[i]], 
           y1 = y[bitReverse[i]], 
           x2 = x[bitReverse[NumSamples-i]], 
           y2 = y[bitReverse[NumSamples-i]],
	   aa,bb;
    a[i] = sqrt(aa= (x1+x2)*(x1+x2) + (y1-y2)*(y1-y2) );
    b[i] = sqrt(bb= (x1-x2)*(x1-x2) + (y1+y2)*(y1+y2) );
    if (aa+bb != 0.0)
      clarity[i] = (int)(
        ( (x1+x2) * (x1-x2) + (y1+y2) * (y1-y2) )/(aa+bb) * 256 );
    else
      clarity[i] = 0;

    energy += (aa+bb)*i*i;
  } 

  energy = sqrt(energy/NumSamples)/65536.0;
   
  //int heightFactor = NumSamples/2 / outHeight + 1;
  //int actualHeight = NumSamples/2/heightFactor;
  //int heightAdd = outHeight + actualHeight >> 1;

  /* Correct for window size */
  //double brightFactor2 = (brightFactor/65536.0/NumSamples)*
  //    sqrt(actualHeight*outWidth/(320.0*200.0));
  double brightFactor2 = (brightFactor/65536.0/NumSamples)*
      sqrt(outHeight*outWidth/(320.0*200.0));

  // Some more arbitrary constants,
  // this time for dynamic range compression of the display
  // (breath slowly, relax, please don't hunt me down and kill me)
  static double energy_avg = 80.0;
  energy_avg = energy_avg*0.95 + energy*0.05;
  if (energy_avg > 0.0)
    brightFactor2 *= 80.0/(energy_avg + 5.0);

  for(i=1;i<NumSamples/2;i++) {
    //int h = (int)( b[i]*280 / (a[i]+b[i]+0.0001)+20 );
    if (a[i] > 0 || b[i] > 0) {
      int h = (int)( b[i]*outWidth / (a[i]+b[i]) );
      int br1, br2, br = (int)( 
          // In 2.4 was: (a[i]+b[i])*i*brightFactor2 );
          (a[i]+b[i])*binScale[i]*brightFactor2 );
      br1 = br*(clarity[i]+128)>>8;
      br2 = br*(128-clarity[i])>>8;

      if (br1 < 0) br1 = 0; else if (br1 > 255) br1 = 255;
      if (br2 < 0) br2 = 0; else if (br2 > 255) br2 = 255;

      int px = h, 
          //In 2.4 was: py = outHeight-i*outHeight/(NumSamples/2);
          py = (int)(binToVert[i] * (double)(outHeight - 1) + 0.5);
          //was heightAdd - i / heightFactor;

      if (pointsAreDiamonds) {
        addPixel(px,py,br1,br2);
        br1=scaleDown[br1];br2=scaleDown[br2];

        //TODO: Use addpixelfast
	for(j=1;br1>0||br2>0;j++,br1=scaleDown[br1],br2=scaleDown[br2]) {
	  for(k=0;k<j;k++) {
	    addPixel(px-j+k,py-k,br1,br2); 
	    addPixel(px+k,py-j+k,br1,br2); 
	    addPixel(px+j-k,py+k,br1,br2); 
	    addPixel(px-k,py+j-k,br1,br2); 
	  }
	}
      } else {
	if (px < maxStarRadius || py < maxStarRadius || 
	    px > outWidth-maxStarRadius || py > outHeight-maxStarRadius) {
	  addPixel(px,py,br1,br2);
	  for(j=1;br1>0||br2>0;j++,br1=scaleDown[br1],br2=scaleDown[br2]) {
	    addPixel(px+j,py,br1,br2);
	    addPixel(px,py+j,br1,br2);
	    addPixel(px-j,py,br1,br2);
	    addPixel(px,py-j,br1,br2);
	  }
	} else {
	  unsigned char *p = output+px*2+py*outWidth*2, *p1=p, *p2=p, *p3=p, *p4=p;
	  addPixelFast(p,br1,br2);
	  for(;br1>0||br2>0;br1=scaleDown[br1],br2=scaleDown[br2]) {
	    p1 += 2;
	    addPixelFast(p1,br1,br2);
	    p2 -= 2;
	    addPixelFast(p2,br1,br2);
	    p3 += outWidth*2;
	    addPixelFast(p3,br1,br2);
	    p4 -= outWidth*2;
	    addPixelFast(p4,br1,br2);
	  }
	}
      }
    }
  }
  return 0;
}
