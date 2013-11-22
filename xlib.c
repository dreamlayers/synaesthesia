/* 
 *     XaoS, a fast portable realtime fractal zoomer 
 *                  Copyright (C) 1996,1997 by
 *
 *      Jan Hubicka          (hubicka@paru.cas.cz)
 *      Thomas Marsh         (tmarsh@austin.ibm.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 * Shamelessly ripped for use in xsynaesthesia
 */
#include "config.h"
#ifndef X_DISPLAY_MISSING

#define X11_DRIVER
//#define MITSHM

#ifdef X11_DRIVER
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#ifndef __FreeBSD__
#include <malloc.h>
#else
#include <stdlib.h>
#endif
#include "xlib.h"
#ifdef AMIGA
#define XFlush(x) while(0)
#endif

#undef PIXMAP

#define chkalloc(n) if (!n) fprintf(stderr, "out of memory\n"), exit(-1)

int xupdate_size(xdisplay * d)
{
    int tmp;
    Window wtmp;
    int width = d->width, height = d->height;
    XGetGeometry(d->display, d->window, &wtmp, &tmp, &tmp, &d->width, &d->height, (unsigned int *) &tmp, (unsigned int *) &tmp);
    if (d->width != width || d->height != height)
	return 1;
    return 0;
}

void xflip_buffers(xdisplay * d)
{
    d->back = d->vbuffs[d->current];
    d->current ^= 1;
    d->vbuff = d->vbuffs[d->current];
}

void draw_screen(xdisplay * d)
{
    switch (d->image[0]->bits_per_pixel) {
    case 16:{
	    uint16_t *de;
	    unsigned char *s;
	    unsigned char *e;
	    for (s = (unsigned char *) d->vbuffs[d->current],
		 e = (unsigned char *) d->vbuffs[d->current] + (d->linewidth * d->height),
		 de = (unsigned short *) d->data[d->current]; s < e; s += 8, de += 8)
		*de = d->pixels[*s],
		    *(de + 1) = d->pixels[*(s + 1)],
		    *(de + 2) = d->pixels[*(s + 2)],
		    *(de + 3) = d->pixels[*(s + 3)],
		    *(de + 4) = d->pixels[*(s + 4)],
		    *(de + 5) = d->pixels[*(s + 5)],
		    *(de + 6) = d->pixels[*(s + 6)],
		    *(de + 7) = d->pixels[*(s + 7)];
	    s -= 8;
	    de -= 8;
	    for (; s < e; s++, de++)
		*de = d->pixels[*s];
	    break;
	}
    case 24:{
	    unsigned char *de;
	    unsigned char *s;
	    unsigned char *e;
	    for (s = (unsigned char *) d->vbuffs[d->current],
		 e = (unsigned char *) d->vbuffs[d->current] + (d->linewidth * d->height),
		 de = (unsigned char *) d->data[d->current]; s < e; s++, de+=3)
	      de[0] = d->pixels[*s],
	      de[1] = d->pixels[*s]>>8,
	      de[2] = d->pixels[*s]>>16;
	    
            break;	
	}
    case 32:{
	    uint32_t *de;
	    unsigned char *s;
	    unsigned char *e;
	    for (s = (unsigned char *) d->vbuffs[d->current],
		 e = (unsigned char *) d->vbuffs[d->current] + (d->linewidth * d->height),
		 de = (uint32_t *) d->data[d->current]; s < e; s += 8, de += 8)
		*de = d->pixels[*s],
		    *(de + 1) = d->pixels[*(s + 1)],
		    *(de + 2) = d->pixels[*(s + 2)],
		    *(de + 3) = d->pixels[*(s + 3)],
		    *(de + 4) = d->pixels[*(s + 4)],
		    *(de + 5) = d->pixels[*(s + 5)],
		    *(de + 6) = d->pixels[*(s + 6)],
		    *(de + 7) = d->pixels[*(s + 7)];
	    s -= 8;
	    de -= 8;
	    for (; s < e; s++, de++)
		*de = d->pixels[*s];
	    break;
	}
    }
#ifdef MITSHM
    if (d->SharedMemFlag) {
	XShmPutImage(d->display, d->window, d->gc, d->image[d->current], 0, 0, 0,
		     0, d->width, d->height, True);
	XFlush(d->display);
    } else
#endif
    {
	XPutImage(d->display, d->window, d->gc, d->image[d->current], 0, 0, 0, 0, d->width, d->height);
	XFlush(d->display);
    }
    d->screen_changed = 0;
}

#ifdef MITSHM
int alloc_shm_image(xdisplay * new)
{
    register char *ptr;
    int temp, size = 0, i;
    ptr = DisplayString(new->display);
    if (!ptr || (*ptr == ':') || !strncmp(ptr, "localhost:", 10) ||
	!strncmp(ptr, "unix:", 5) || !strncmp(ptr, "local:", 6)) {
	new->SharedMemOption = XQueryExtension(new->display, "MIT-SHM", &temp, &temp, &temp);
    } else {
	new->SharedMemOption = False;
	return 0;
    }
    new->SharedMemFlag = False;
#if 0
    new->SharedMemOption = True;
    new->SharedMemFlag = False;
#endif

    if (new->SharedMemFlag) {
	XShmDetach(new->display, &new->xshminfo[0]);
	XShmDetach(new->display, &new->xshminfo[1]);
	new->image[0]->data = (char *) NULL;
	new->image[1]->data = (char *) NULL;
	shmdt(new->xshminfo[0].shmaddr);
	shmdt(new->xshminfo[1].shmaddr);
    }
    for (i = 0; i < 2; i++) {
	if (new->SharedMemOption) {
	    int mul;
	    if (new->depth == 8)
		mul = 1;
	    else if (new->depth <= 24)
		mul = 2;
	    else
		mul = 4;
	    new->SharedMemFlag = False;
	    new->image[i] = XShmCreateImage(new->display, new->visual, new->depth, ZPixmap,
		 NULL, &new->xshminfo[i], new->width, new->height * mul);
	    if (new->image[i]) {
		temp = new->image[i]->bytes_per_line * new->image[i]->height;
		new->linewidth = new->image[i]->bytes_per_line * 8 / new->image[i]->bits_per_pixel;
		if (temp > size)
		    size = temp;
		new->xshminfo[i].shmid = shmget(IPC_PRIVATE, size, IPC_CREAT | 0777);
		if (new->xshminfo[i].shmid != -1) {
		    new->xshminfo[i].shmaddr = (char *) shmat(new->xshminfo[i].shmid, 0, 0);
		    if (new->xshminfo[i].shmaddr != (char *) -1) {
			new->image[i]->data = new->xshminfo[i].shmaddr;
			new->data[i] = new->vbuffs[i] = (char *) new->image[i]->data;
			new->xshminfo[i].readOnly = True;

			new->SharedMemFlag = XShmAttach(new->display, &new->xshminfo[i]);
			XSync(new->display, False);
			if (!new->SharedMemFlag) {
			    XDestroyImage(new->image[i]);
			    new->image[i] = (XImage *) NULL;
			    new->SharedMemFlag = 0;
			    return 0;
			}
		    }
		    /* Always Destroy Shared Memory Ident */
		    shmctl(new->xshminfo[i].shmid, IPC_RMID, 0);
		}
		if (!new->SharedMemFlag) {
		    XDestroyImage(new->image[i]);
		    new->image[i] = (XImage *) NULL;
		    new->SharedMemFlag = 0;
		    return 0;
		}
	    } else {
		new->SharedMemFlag = 0;
		return 0;
	    }
	} else {
	    new->SharedMemFlag = 0;
	    return 0;
	}
    }
    new->current = 0;
    xflip_buffers(new);
    return 1;
}

void free_shm_image(xdisplay * d)
{
    if (d->SharedMemFlag) {
	XDestroyImage(d->image[0]);
	XDestroyImage(d->image[1]);
	XShmDetach(d->display, &d->xshminfo[0]);
	XShmDetach(d->display, &d->xshminfo[1]);
	shmdt(d->xshminfo[0].shmaddr);
	shmdt(d->xshminfo[1].shmaddr);
    }
}

#endif

int alloc_image(xdisplay * d)
{
    int i;
#ifdef MITSHM
    if (!d->params->nomitshm && alloc_shm_image(d)) {
	if (d->depth != 8) {
	    for (i = 0; i < 2; i++)
		d->vbuffs[i] = malloc(d->linewidth * d->height);
	}
	return 1;
    }
#endif
    for (i = 0; i < 2; i++) {
	d->image[i] = XCreateImage(d->display, d->visual, d->depth, ZPixmap, 0,
				   NULL, d->width, d->height, 8, 0);
	if (d->image[i] == NULL) {
	    printf("Out of memory for image..exiting\n");
	    exit(-1);
	}
	//Add a little extra memory to catch overruns when dumping image to buffer in draw_screen
	d->image[i]->data = malloc(d->image[i]->bytes_per_line * d->height + 32);
	memset(d->image[i]->data,0,d->image[i]->bytes_per_line * d->height);

	if (d->image[i]->data == NULL) {
	    printf("Out of memory for image buffers..exiting\n");
	    exit(-1);
	}
	d->data[i] = d->vbuffs[i] = (char *) d->image[i]->data;
	d->linewidth = d->image[i]->bytes_per_line * 8 / d->image[i]->bits_per_pixel;
    }
    if (d->depth != 8) {
	for (i = 0; i < 2; i++) {
	    //Add a little extra memory to catch overruns 
	    //when dumping image to buffer in draw_screen
	    d->vbuffs[i] = malloc(d->linewidth * d->height + 32);
	    memset(d->vbuffs[i],0,d->linewidth * d->height);

	    if (d->vbuffs[i] == NULL) {
		printf("Out of memory for image buffers2..exiting\n");
		exit(-1);
	    }
	}
    }
    xflip_buffers(d);
    return 1;
}

void free_image(xdisplay * d)
{
    if (d->depth != 8)
	free(d->vbuffs[0]), free(d->vbuffs[1]);
#ifdef MITSHM
    if (d->SharedMemFlag) {
	free_shm_image(d);
	return;
    }
#endif
    XDestroyImage(d->image[0]);
    XDestroyImage(d->image[1]);
}
#define MAX(x,y) ((x)>(y)?(x):(y))


xdisplay *xalloc_display(char *s, int xHint, int yHint, int x, int y, xlibparam * params)
{
    xdisplay *new;
    Visual *defaultvisual;
    XVisualInfo vis;


    new = (xdisplay *) calloc(sizeof(xdisplay), 1);
    chkalloc(new);
    new->display = XOpenDisplay((char *) NULL);
    if (!new->display) {
	free((void *) new);
	return NULL;
    }
    new->screen = DefaultScreen(new->display);
    new->attributes = (XSetWindowAttributes *)
	malloc(sizeof(XSetWindowAttributes));
    chkalloc(new->attributes);
    new->attributes->background_pixel = BlackPixel(new->display,
						   new->screen);
    new->attributes->border_pixel = BlackPixel(new->display, new->screen);
    new->attributes->event_mask = ButtonPressMask | StructureNotifyMask | ButtonReleaseMask | ButtonMotionMask | KeyPressMask | ExposureMask | KeyReleaseMask;
    new->attributes->override_redirect = False;
    new->attr_mask = CWBackPixel | CWBorderPixel | CWEventMask;
    new->classX = InputOutput;
    new->xcolor.n = 0;
    new->parent_window = RootWindow(new->display, new->screen);
    defaultvisual = DefaultVisual(new->display, new->screen);
    new->params = params;
    if (!params->usedefault) {
	if (defaultvisual->class != PseudoColor || (!XMatchVisualInfo(new->display, new->screen, 8, PseudoColor, &vis) && vis.colormap_size > 128)) {
	    new->fixedcolormap = 1;
	    if (!XMatchVisualInfo(new->display, new->screen, 15, TrueColor, &vis)) {
		if (!XMatchVisualInfo(new->display, new->screen, 16, TrueColor, &vis)) {
		    if (!XMatchVisualInfo(new->display, new->screen, 32, TrueColor, &vis) &&
			!XMatchVisualInfo(new->display, new->screen, 24, TrueColor, &vis)) {
			if (!XMatchVisualInfo(new->display, new->screen, 8, PseudoColor, &vis) &&
			    !XMatchVisualInfo(new->display, new->screen, 7, PseudoColor, &vis)) {
			    if (!XMatchVisualInfo(new->display, new->screen, 8, TrueColor, &vis) &&
				!XMatchVisualInfo(new->display, new->screen, 8, StaticColor, &vis) &&
				!XMatchVisualInfo(new->display, new->screen, 8, StaticGray, &vis)) {
				printf("Display does not support PseudoColor depth 7,8,StaticColor depth 8, StaticGray depth 8, Truecolor depth 8,15,16,24 nor 32!\n");
				return NULL;
			    } else
				new->truecolor = 1;
			} else
			    new->fixedcolormap = 0, new->truecolor = 0;
		    } else
			new->truecolor = 1;
		} else
		    new->truecolor = 1;
	    } else
		new->truecolor = 1;
	} else {
	    new->truecolor = 0;
	}
	new->depth = vis.depth;
	new->visual = vis.visual;
    } else {			/*usedefault */
	vis.depth = new->depth = DefaultDepth(new->display, new->screen);
	new->visual = defaultvisual;
	switch (defaultvisual->class) {
	case PseudoColor:
	    if (new->depth <= 8) {
		new->depth = 8;
		new->truecolor = 0;
		new->fixedcolormap = 0;
	    } else {
		printf("Pseudocolor visual on unsuported depth\n");
		return NULL;
	    }
	    break;
	case TrueColor:
	case StaticColor:
	case StaticGray:
	    new->truecolor = 1;
	    new->fixedcolormap = 1;
	    if (new->depth <= 8)
		new->depth = 8;
	    else if (new->depth <= 16)
		new->depth = 16;
	    else if (new->depth <= 32)
		new->depth = 32;
	    else {
		printf("Truecolor visual on unsuported depth\n");
		return NULL;
	    }
	    break;
	default:
	    printf("Unusuported visual\n");
	    break;
	}
    }
    /*new->visual->map_entries = 256; */
    new->colormap = new->defaultcolormap = DefaultColormap(new->display, new->screen);

    new->window_name = s;
    new->height = y;
    new->width = x;
    new->border_width = 2;
    new->lastx = 0;
    new->lasty = 0;
    new->font_struct = (XFontStruct *) NULL;

    new->window = XCreateWindow(new->display, new->parent_window, xHint, yHint,
			      new->width, new->height, new->border_width,
				vis.depth, new->classX, new->visual,
				new->attr_mask, new->attributes);
    if (!new->fixedcolormap && params->privatecolormap) {
	unsigned long pixels[256];
	int i;
	new->colormap = XCreateColormap(new->display, new->window, new->visual, AllocNone);
	XAllocColorCells(new->display, new->colormap, 1, 0, 0, pixels, MAX(new->visual->map_entries, 256));
	for (i = 0; i < 16; i++) {
	    new->xcolor.c[i].pixel = pixels[i];
	}
	XQueryColors(new->display, new->defaultcolormap, new->xcolor.c, 16);
	XStoreColors(new->display, new->colormap, new->xcolor.c, 16);
	new->privatecolormap = 1;
    }
    if (!new->fixedcolormap)
	XSetWindowColormap(new->display, new->window, new->colormap);
    new->gc = XCreateGC(new->display, new->window, 0L, &(new->xgcvalues));
    XSetBackground(new->display, new->gc,
		   BlackPixel(new->display, new->screen));
    XSetForeground(new->display, new->gc,
		   WhitePixel(new->display, new->screen));
    XStoreName(new->display, new->window, new->window_name);
    XMapWindow(new->display, new->window);
#if 1
    XSelectInput(new->display, new->window, 
                 //ExposureMask | 
		 KeyPress |
		 //KeyRelease | 
                 //ConfigureRequest | 
		 //FocusChangeMask | 
		 StructureNotifyMask |
                 ButtonPressMask | ButtonReleaseMask);
#endif
#ifdef PIXAMP
    new->pixmap = XCreatePixmap(new->display, new->window, new->width,
				new->height, new->depth);
#endif

    {
      XColor c;
      Pixmap p = XCreatePixmap(new->display, new->window, 1,1,1);
      memset(&c,0,sizeof(c));
      new->cursor = XCreatePixmapCursor(new->display, p,p,
        &c,&c, 0,0);
      XDefineCursor(new->display,new->window,new->cursor);
      XFreePixmap(new->display, p);
    }

    return (new);
}

void xsetcolor(xdisplay * d, int col)
{
    switch (col) {
    case 0:
	XSetForeground(d->display, d->gc,
		       BlackPixel(d->display, d->screen));
	break;
    case 1:
	XSetForeground(d->display, d->gc,
		       WhitePixel(d->display, d->screen));
	break;
    default:
	if ((col - 2) > d->xcolor.n) {
	    fprintf(stderr, "color error\n");
	    exit(-1);
	}
	XSetForeground(d->display, d->gc,
		       d->xcolor.c[col - 2].pixel);
	break;
    }
}
void xrotate_palette(xdisplay * d, int direction, unsigned char co[3][256], int ncolors)
{
    int i, p;

    if (d->privatecolormap) {
	for (i = 0; i < d->xcolor.n; i++) {
	    p = d->xcolor.c[i].pixel;
	    d->xcolor.c[i].red = (int) co[0][p] * 256;
	    d->xcolor.c[i].green = (int) co[1][p] * 256;
	    d->xcolor.c[i].blue = (int) co[2][p] * 256;
	}
	XStoreColors(d->display, d->colormap, d->xcolor.c, d->xcolor.n);
    }
    if (d->truecolor) {
	unsigned long oldpixels[256];
	memcpy(oldpixels, d->pixels, sizeof(oldpixels));
	p = (ncolors - 1 + direction) % (ncolors - 1) + 1;
	for (i = 1; i < ncolors; i++) {		/*this is ugly..I know */
	    d->pixels[i] = oldpixels[p];
	    p++;
	    if (p >= ncolors)
		p = 1;
	}
	draw_screen(d);
    }
}
int xalloc_color(xdisplay * d, int r, int g, int b, int readwrite)
{
    d->xcolor.n++;
    d->xcolor.c[d->xcolor.n - 1].flags = DoRed | DoGreen | DoBlue;
    d->xcolor.c[d->xcolor.n - 1].red = r;
    d->xcolor.c[d->xcolor.n - 1].green = g;
    d->xcolor.c[d->xcolor.n - 1].blue = b;
    d->xcolor.c[d->xcolor.n - 1].pixel = d->xcolor.n - 1;
    if ((readwrite && !d->fixedcolormap) || d->privatecolormap) {
	unsigned long cell;
	if (d->privatecolormap) {
	    cell = d->xcolor.c[d->xcolor.n - 1].pixel += 16;
	    if (d->xcolor.c[d->xcolor.n - 1].pixel >= d->visual->map_entries) {
		d->xcolor.n--;
		return (-1);
	    }
	} else {
	    if (!XAllocColorCells(d->display, d->colormap, 0, 0, 0, &cell, 1)) {
		d->xcolor.n--;
		if (d->xcolor.n <= 32)
		    printf("Colormap is too full! close some colorfull aplications or use -private\n");
		return (-1);
	    }
	    d->xcolor.c[d->xcolor.n - 1].pixel = cell;
	}
	XStoreColor(d->display, d->colormap, &(d->xcolor.c[d->xcolor.n - 1]));
	return (cell);
    }
    if (!XAllocColor(d->display, d->colormap, &(d->xcolor.c[d->xcolor.n - 1]))) {
	d->xcolor.n--;
	if (d->xcolor.n <= 32)
	    printf("Colormap is too full! close some colorfull aplications or use -private\n");
	return (-1);
    }
    d->pixels[d->xcolor.n - 1] = d->xcolor.c[d->xcolor.n - 1].pixel;
    return (d->depth != 8 ? d->xcolor.n - 1 : d->xcolor.c[d->xcolor.n - 1].pixel);
}

void xfree_colors(xdisplay * d)
{
    unsigned long pixels[256];
    int i;
    for (i = 0; i < d->xcolor.n; i++)
	pixels[i] = d->xcolor.c[i].pixel;
    if (!d->privatecolormap)
	XFreeColors(d->display, d->colormap, pixels, d->xcolor.n, 0);
    d->xcolor.n = 0;
}

void xfree_display(xdisplay * d)
{
    XSync(d->display, 0);
    if (d->font_struct != (XFontStruct *) NULL) {
	XFreeFont(d->display, d->font_struct);
    }
    XUnmapWindow(d->display, d->window);
#ifdef PIXMAP
    XFreePixmap(d->display, d->pixmap);
#endif
    XDestroyWindow(d->display, d->window);
    XFreeCursor(d->display, d->cursor);
    XCloseDisplay(d->display);
    free((void *) d->attributes);
    free((void *) d);
}

#ifdef PIXMAP
void xline(xdisplay * d, int x1, int y1, int x2, int y2)
{
    XDrawLine(d->display, d->pixmap, d->gc, x1, y1, x2, y2);
    d->lastx = x2, d->lasty = y2;
    d->screen_changed = 1;
} void xlineto(xdisplay * d, int x, int y)
{

    XDrawLine(d->display, d->pixmap, d->gc, d->lastx, d->lasty, x, y);
    d->lastx = x, d->lasty = y;
    d->screen_changed = 1;
} void xrect(xdisplay * d, int x1, int y1, int x2, int y2)
{

    XDrawRectangle(d->display, d->pixmap, d->gc, x1, y1,
		   (x2 - x1), (y2 - y1));
    d->lastx = x2, d->lasty = y2;
    d->screen_changed = 1;
} void xfillrect(xdisplay * d, int x1, int y1, int x2, int y2)
{

    XFillRectangle(d->display, d->pixmap, d->gc, x1, y1,
		   (x2 - x1), (y2 - y1));
    d->lastx = x2, d->lasty = y2;
    d->screen_changed = 1;
} void xpoint(xdisplay * d, int x, int y)
{

    XDrawPoint(d->display, d->pixmap, d->gc, x, y);
    d->lastx = x, d->lasty = y;
    d->screen_changed = 1;
} void xflush(xdisplay * d)
{

    draw_screen(d);
    XFlush(d->display);
}

void xclear_screen(xdisplay * d)
{
    xfillrect(d, 0, 0, d->width, d->height);
    d->screen_changed = 1;
}

#endif
void xmoveto(xdisplay * d, int x, int y)
{
    d->lastx = x, d->lasty = y;
} int xsetfont(xdisplay * d, char *font_name)
{

    if (d->font_struct != (XFontStruct *) NULL) {
	XFreeFont(d->display, d->font_struct);
    }
    d->font_struct = XLoadQueryFont(d->display, font_name);
    if (!d->font_struct) {
	fprintf(stderr, "could not load font: %s\n", font_name);
	exit(-1);
    }
    return (d->font_struct->max_bounds.ascent + d->font_struct->max_bounds.descent);
}

void xouttext(xdisplay * d, char *string)
{
    int sz;

    sz = strlen(string);
    XDrawImageString(d->display, d->window, d->gc, d->lastx, d->lasty,
		     string, sz);
#if 0
    d->lastx += XTextWidth(d->font_struct, string, sz);
    d->screen_changed = 1;
#endif
} void xresize(xdisplay * d, XEvent * ev)
{

#ifdef PIXMAP
    XFreePixmap(d->display, d->pixmap);
#endif
    d->width = ev->xconfigure.width;
    d->height = ev->xconfigure.height;
#ifdef PIXMAP
    d->pixmap = XCreatePixmap(d->display, d->window, d->width,
			      d->height, d->depth);
#endif
}

#ifdef PIXMAP
void xarc(xdisplay * d, int x, int y, unsigned int w,
	  unsigned int h, int a1, int a2)
{
    XDrawArc(d->display, d->pixmap, d->gc, x, y, w, h, a1, a2);
} void xfillarc(xdisplay * d, int x, int y, unsigned int w,
		unsigned int h, int a1, int a2)
{
    XFillArc(d->display, d->pixmap, d->gc, x, y, w, h, a1, a2);
} 
#endif

void xsize_set(xdisplay *d, int width, int height)
{
  XResizeWindow(d->display, d->window, width, height);
}

int xmouse_x(xdisplay * d)
{

    return d->mouse_x;
}

int xmouse_y(xdisplay * d)
{
    return d->mouse_y;
}

void xmouse_update(xdisplay * d)
{
    Window rootreturn, childreturn;
    int rootx = 0, rooty = 0, buttons = 0;

    XEvent event;
  
    if (XCheckMaskEvent(d->display,ButtonPressMask | ButtonReleaseMask, &event)) {
      if (event.type == ButtonPress)
        d->mouse_buttons |= 1 << ((XButtonEvent*)(&event))->button;
      else
        d->mouse_buttons &= ~( 1 << ((XButtonEvent*)(&event))->button );
    }

    XQueryPointer(d->display, d->window, &rootreturn, &childreturn,
		  &rootx, &rooty, &(d->mouse_x), &(d->mouse_y),
		  &buttons); 
}

char xkeyboard_query(xdisplay * d) {
    XEvent event;
  
    if (XCheckMaskEvent(d->display,KeyPressMask | KeyReleaseMask, &event)) {
      char *str =
        XKeysymToString(XLookupKeysym((XKeyPressedEvent*)(&event),0));
        
      if ( ((XKeyPressedEvent*)(&event))->state & 
	   (ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask) )
	return 0;

      if (str) {
        char key;

        if (strlen(str) == 1)
	  key = str[0];
	else if (strcmp(str,"equal") == 0)
	  key = '=';
	else if (strcmp(str,"minus") == 0)
	  key = '-';
	else if (strcmp(str,"bracketleft") == 0)
	  key = '[';
	else if (strcmp(str,"bracketright") == 0)
	  key = ']';
	else if (strcmp(str,"comma") == 0)
	  key = ',';
	else if (strcmp(str,"period") == 0)
	  key = '.';
	else if (strcmp(str,"slash") == 0)
	  key = '/';
	else return 0;
	
        if ( ((XKeyPressedEvent*)(&event))->state & ShiftMask )
	  switch(key) {
	    case '=' : key = '+'; break;
	    case '[' : key = '{'; break;
	    case ']' : key = '}'; break;
	    case ',' : key = '<'; break;
	    case '/' : key = '?'; break;
	    default :
	      if (key >= 'a' && key <= 'z')
		key = key+'A'-'a';
	      break;
	  }
        return key;
      }
    }

    return 0;
}

int xsize_update(xdisplay *d,int *width,int *height) {
    XEvent event;
  
    if (XCheckMaskEvent(d->display,StructureNotifyMask, &event)) {
      if (event.type == ConfigureNotify) {
        xupdate_size(d);
        free_image(d);
        alloc_image(d);
        *width = d->linewidth;
        *height = d->height;
        return 1;
      }
    }

    return 0;
}

unsigned int xmouse_buttons(xdisplay * d)
{
    return d->mouse_buttons;
}
#endif

#endif
