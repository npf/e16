/*
 * Copyright (C) 2000-2005 Carsten Haitzler, Geoff Harrison and various contributors
 * Copyright (C) 2004-2005 Kim Woelders
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies of the Software, its documentation and marketing & publicity
 * materials, and acknowledgment shall be given in the documentation, materials
 * and software packages that this Software was used.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <time.h>
#include "E.h"
#include "desktops.h"
#include "emodule.h"
#include "iclass.h"
#include "tclass.h"
#include "xwin.h"

typedef struct
{
   char               *file;
   char               *real_file;
   Imlib_Image        *im;
   char                keep_aspect;
   int                 xjust, yjust;
   int                 xperc, yperc;
} BgPart;

struct _background
{
   char               *name;
   Pixmap              pmap;
   time_t              last_viewed;
   XColor              bg_solid;
   char                bg_tile;
   BgPart              bg;
   BgPart              top;
#if ENABLE_COLOR_MODIFIERS
   ColorModifierClass *cmclass;
#endif
   char                keepim;
   unsigned int        ref_count;
};

char               *
BackgroundGetUniqueString(Background * bg)
{
   char                s[256];
   const char         *chmap =
      "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ-_";
   int                 r, g, b;
   int                 n1, n2, n3, n4, n5, f1, f2, f3, f4, f5, f6;

   EGetColor(&(bg->bg_solid), &r, &g, &b);
   n1 = (r << 24) | (g << 16) | (b << 8) | (bg->bg_tile << 7)
      | (bg->bg.keep_aspect << 6) | (bg->top.keep_aspect << 5);
   n2 = (bg->bg.xjust << 16) | (bg->bg.yjust);
   n3 = (bg->bg.xperc << 16) | (bg->bg.yperc);
   n4 = (bg->top.xjust << 16) | (bg->top.yjust);
   n5 = (bg->top.xperc << 16) | (bg->top.yperc);
   f1 = 0;
   f2 = 0;
   f3 = 0;
   f4 = 0;
   f5 = 0;
   f6 = 0;
   if (bg->bg.file)
     {
	char               *f;

	f = ThemeFileFind(bg->bg.file, 0);
	if (f)
	  {
	     f1 = fileinode(f);
	     f2 = filedev(f);
	     f3 = (int)moddate(f);
	     Efree(f);
	  }
     }
   if (bg->top.file)
     {
	char               *f;

	f = ThemeFileFind(bg->top.file, 0);
	if (f)
	  {
	     f4 = fileinode(f);
	     f5 = filedev(f);
	     f6 = (int)moddate(f);
	     Efree(f);
	  }
     }
   Esnprintf(s, sizeof(s),
	     "%c%c%c%c%c%c" "%c%c%c%c%c%c" "%c%c%c%c%c%c" "%c%c%c%c%c%c"
	     "%c%c%c%c%c%c" "%c%c%c%c%c%c" "%c%c%c%c%c%c" "%c%c%c%c%c%c"
	     "%c%c%c%c%c%c" "%c%c%c%c%c%c" "%c%c%c%c%c%c",
	     chmap[(n1 >> 0) & 0x3f], chmap[(n1 >> 6) & 0x3f],
	     chmap[(n1 >> 12) & 0x3f], chmap[(n1 >> 18) & 0x3f],
	     chmap[(n1 >> 24) & 0x3f], chmap[(n1 >> 28) & 0x3f],
	     chmap[(n2 >> 0) & 0x3f], chmap[(n2 >> 6) & 0x3f],
	     chmap[(n2 >> 12) & 0x3f], chmap[(n2 >> 18) & 0x3f],
	     chmap[(n2 >> 24) & 0x3f], chmap[(n2 >> 28) & 0x3f],
	     chmap[(n3 >> 0) & 0x3f], chmap[(n3 >> 6) & 0x3f],
	     chmap[(n3 >> 12) & 0x3f], chmap[(n3 >> 18) & 0x3f],
	     chmap[(n3 >> 24) & 0x3f], chmap[(n3 >> 28) & 0x3f],
	     chmap[(n4 >> 0) & 0x3f], chmap[(n4 >> 6) & 0x3f],
	     chmap[(n4 >> 12) & 0x3f], chmap[(n4 >> 18) & 0x3f],
	     chmap[(n4 >> 24) & 0x3f], chmap[(n4 >> 28) & 0x3f],
	     chmap[(n5 >> 0) & 0x3f], chmap[(n5 >> 6) & 0x3f],
	     chmap[(n5 >> 12) & 0x3f], chmap[(n5 >> 18) & 0x3f],
	     chmap[(n5 >> 24) & 0x3f], chmap[(n5 >> 28) & 0x3f],
	     chmap[(f1 >> 0) & 0x3f], chmap[(f1 >> 6) & 0x3f],
	     chmap[(f1 >> 12) & 0x3f], chmap[(f1 >> 18) & 0x3f],
	     chmap[(f1 >> 24) & 0x3f], chmap[(f1 >> 28) & 0x3f],
	     chmap[(f2 >> 0) & 0x3f], chmap[(f2 >> 6) & 0x3f],
	     chmap[(f2 >> 12) & 0x3f], chmap[(f2 >> 18) & 0x3f],
	     chmap[(f2 >> 24) & 0x3f], chmap[(f2 >> 28) & 0x3f],
	     chmap[(f3 >> 0) & 0x3f], chmap[(f3 >> 6) & 0x3f],
	     chmap[(f3 >> 12) & 0x3f], chmap[(f3 >> 18) & 0x3f],
	     chmap[(f3 >> 24) & 0x3f], chmap[(f3 >> 28) & 0x3f],
	     chmap[(f4 >> 0) & 0x3f], chmap[(f4 >> 6) & 0x3f],
	     chmap[(f4 >> 12) & 0x3f], chmap[(f4 >> 18) & 0x3f],
	     chmap[(f4 >> 24) & 0x3f], chmap[(f4 >> 28) & 0x3f],
	     chmap[(f5 >> 0) & 0x3f], chmap[(f5 >> 6) & 0x3f],
	     chmap[(f5 >> 12) & 0x3f], chmap[(f5 >> 18) & 0x3f],
	     chmap[(f5 >> 24) & 0x3f], chmap[(f5 >> 28) & 0x3f],
	     chmap[(f6 >> 0) & 0x3f], chmap[(f6 >> 6) & 0x3f],
	     chmap[(f6 >> 12) & 0x3f], chmap[(f6 >> 18) & 0x3f],
	     chmap[(f6 >> 24) & 0x3f], chmap[(f6 >> 28) & 0x3f]);
   return Estrdup(s);
}

void
BackgroundPixmapFree(Background * bg)
{
   if (bg && bg->pmap)
     {
	imlib_free_pixmap_and_mask(bg->pmap);
	bg->pmap = 0;
     }
}

void
BackgroundImagesFree(Background * bg, int free_pmap)
{
   if (bg->bg.im)
     {
	imlib_context_set_image(bg->bg.im);
	imlib_free_image();
	bg->bg.im = NULL;
     }
   if (bg->top.im)
     {
	imlib_context_set_image(bg->top.im);
	imlib_free_image();
	bg->top.im = NULL;
     }
   if (free_pmap && bg->pmap)
      BackgroundPixmapFree(bg);
}

static void
BackgroundImagesKeep(Background * bg, int onoff)
{
   if (onoff)
     {
	bg->keepim = 1;
     }
   else
     {
	bg->keepim = 0;
	BackgroundImagesFree(bg, 0);
     }
}

static void
BackgroundImagesRemove(Background * bg)
{
   if (bg->bg.file)
      Efree(bg->bg.file);
   bg->bg.file = NULL;

   if (bg->bg.real_file)
      Efree(bg->bg.real_file);
   bg->bg.real_file = NULL;

   if (bg->top.file)
      Efree(bg->top.file);
   bg->top.file = NULL;

   if (bg->top.real_file)
      Efree(bg->top.real_file);
   bg->top.real_file = NULL;

   BackgroundImagesFree(bg, 1);

   bg->keepim = 0;
}

static int
BackgroundDestroy(Background * bg)
{
   if (!bg)
      return -1;

   if (bg->ref_count > 0)
     {
	DialogOK(_("Background Error!"), _("%u references remain\n"),
		 bg->ref_count);
	return -1;
     }

   RemoveItemByPtr(bg, LIST_TYPE_BACKGROUND);
   BackgroundImagesRemove(bg);

   if (bg->name)
      Efree(bg->name);

   Efree(bg);

   return 0;
}

static void
BackgroundDelete(Background * bg)
{
   char               *f;

   if (BackgroundDestroy(bg))
      return;

   /* And delete the actual image files */
   if (bg->bg.file)
     {
	f = ThemeFileFind(bg->bg.file, 0);
	if (f)
	  {
	     E_rm(f);
	     Efree(f);
	  }
     }
   if (bg->top.file)
     {
	f = ThemeFileFind(bg->top.file, 0);
	if (f)
	  {
	     E_rm(f);
	     Efree(f);
	  }
     }
}

static Background  *
BackgroundCreate(const char *name, XColor * solid, const char *bgn, char tile,
		 char keep_aspect, int xjust, int yjust, int xperc,
		 int yperc, const char *top, char tkeep_aspect, int txjust,
		 int tyjust, int txperc, int typerc)
{
   Background         *bg;

   bg = Emalloc(sizeof(Background));
   if (!bg)
      return NULL;

   bg->name = Estrdup(name);
   bg->pmap = 0;
   bg->last_viewed = 0;

   ESetColor(&(bg->bg_solid), 160, 160, 160);
   if (solid)
      bg->bg_solid = *solid;
   bg->bg.file = NULL;
   if (bgn)
      bg->bg.file = Estrdup(bgn);
   bg->bg.real_file = NULL;
   bg->bg.im = NULL;
   bg->bg_tile = tile;
   bg->bg.keep_aspect = keep_aspect;
   bg->bg.xjust = xjust;
   bg->bg.yjust = yjust;
   bg->bg.xperc = xperc;
   bg->bg.yperc = yperc;

   bg->top.file = NULL;
   if (top)
      bg->top.file = Estrdup(top);
   bg->top.real_file = NULL;
   bg->top.im = NULL;
   bg->top.keep_aspect = tkeep_aspect;
   bg->top.xjust = txjust;
   bg->top.yjust = tyjust;
   bg->top.xperc = txperc;
   bg->top.yperc = typerc;

#if ENABLE_COLOR_MODIFIERS
   bg->cmclass = NULL;
#endif
   bg->keepim = 0;
   bg->ref_count = 0;

   AddItem(bg, bg->name, 0, LIST_TYPE_BACKGROUND);

   return bg;
}

void
BackgroundDestroyByName(const char *name)
{
   BackgroundDestroy(FindItem(name, 0, LIST_FINDBY_NAME, LIST_TYPE_BACKGROUND));
}

static int
BackgroundModify(Background * bg, XColor * solid, const char *bgn, char tile,
		 char keep_aspect, int xjust, int yjust, int xperc,
		 int yperc, const char *top, char tkeep_aspect, int txjust,
		 int tyjust, int txperc, int typerc)
{
   int                 updated = 0;

   if (solid->red != bg->bg_solid.red)
      updated = 1;
   if (solid->green != bg->bg_solid.green)
      updated = 1;
   if (solid->blue != bg->bg_solid.blue)
      updated = 1;
   bg->bg_solid = *solid;

   if ((bg->bg.file) && (bgn))
     {
	if (strcmp(bg->bg.file, bgn))
	   updated = 1;
     }
   else
      updated = 1;
   if (bg->bg.file)
      Efree(bg->bg.file);
   bg->bg.file = (bgn[0]) ? Estrdup(bgn) : NULL;
   if ((int)tile != bg->bg_tile)
      updated = 1;
   if ((int)keep_aspect != bg->bg.keep_aspect)
      updated = 1;
   if (xjust != bg->bg.xjust)
      updated = 1;
   if (yjust != bg->bg.yjust)
      updated = 1;
   if (xperc != bg->bg.xperc)
      updated = 1;
   if (yperc != bg->bg.yperc)
      updated = 1;
   bg->bg_tile = (char)tile;
   bg->bg.keep_aspect = (char)keep_aspect;
   bg->bg.xjust = xjust;
   bg->bg.yjust = yjust;
   bg->bg.xperc = xperc;
   bg->bg.yperc = yperc;

   if ((bg->top.file) && (top))
     {
	if (strcmp(bg->top.file, top))
	   updated = 1;
     }
   else
      updated = 1;
   if (bg->top.file)
      Efree(bg->top.file);
   bg->top.file = (top[0]) ? Estrdup(top) : NULL;
   if ((int)tkeep_aspect != bg->top.keep_aspect)
      updated = 1;
   if (txjust != bg->top.xjust)
      updated = 1;
   if (tyjust != bg->top.yjust)
      updated = 1;
   if (txperc != bg->top.xperc)
      updated = 1;
   if (typerc != bg->top.yperc)
      updated = 1;
   bg->top.keep_aspect = (char)tkeep_aspect;
   bg->top.xjust = txjust;
   bg->top.yjust = tyjust;
   bg->top.xperc = txperc;
   bg->top.yperc = typerc;

   if (updated)
     {
	if (bg->pmap)
	   imlib_free_pixmap_and_mask(bg->pmap);
	bg->pmap = 0;
     }

   return updated;
}

#if ENABLE_COLOR_MODIFIERS
static void
BackgroundSetColorMofifier(Background * bg, ColorModifierClass * cm)
{
   cm->ref_count++;
   bg->cmclass = cm;
}
#endif

static void
BgFindImageSize(BgPart * bgp, unsigned int rw, unsigned int rh,
		unsigned int *pw, unsigned int *ph)
{
   int                 w, h;

   if (bgp->xperc > 0)
      w = (rw * bgp->xperc) >> 10;
   else
      w = (imlib_image_get_width() * rw) / VRoot.w;

   if (bgp->yperc > 0)
      h = (rh * bgp->yperc) >> 10;
   else
      h = (imlib_image_get_height() * rh) / VRoot.h;

   if (w <= 0)
      w = 1;
   if (h <= 0)
      h = 1;

   if (bgp->keep_aspect)
     {
	if (bgp->yperc <= 0)
	  {
	     if (((w << 10) / h) !=
		 ((imlib_image_get_width() << 10) / imlib_image_get_height()))
		h = ((w * imlib_image_get_height()) / imlib_image_get_width());
	  }
	else
	  {
	     if (((h << 10) / w) !=
		 ((imlib_image_get_height() << 10) / imlib_image_get_width()))
		w = ((h * imlib_image_get_width()) / imlib_image_get_height());
	  }
     }

   *pw = (unsigned int)w;
   *ph = (unsigned int)h;
}

static void
e16_tile_image_onto_image(Imlib_Image * tile, int blend, int tw, int th,
			  int dx, int dy, int dw, int dh, int ox, int oy)
{
   Imlib_Image        *im, *tim;
   int                 x, y, tx, ty, ww, hh;
   int                 sw, sh;

   im = imlib_context_get_image();
   imlib_context_set_image(tile);
   sw = imlib_image_get_width();
   sh = imlib_image_get_height();
   if (sw == tw && sh == th)
     {
	tim = tile;
     }
   else
     {
	tim = imlib_create_image(tw, th);
	imlib_context_set_image(tim);
	imlib_context_set_blend(0);
	imlib_blend_image_onto_image(tile, 0, 0, 0, sw, sh, 0, 0, tw, th);
     }
   imlib_context_set_image(im);

   if (ox)
     {
	ox = tw - ox;
	ox %= tw;
	if (ox < 0)
	   ox += tw;
     }
   if (oy)
     {
	oy = th - oy;
	oy %= th;
	if (oy < 0)
	   oy += th;
     }
   dw += dx;
   dh += dy;
   y = dy;
   ty = oy;
   hh = th - oy;
   for (;;)
     {
	if (y + hh >= dh)
	   hh = dh - y;
	if (hh <= 0)
	   break;
	x = dx;
	tx = ox;
	ww = tw - ox;
	for (;;)
	  {
	     if (x + ww >= dw)
		ww = dw - x;
	     if (ww <= 0)
		break;
	     imlib_blend_image_onto_image(tim, blend, tx, ty, ww, hh,
					  x, y, ww, hh);
	     tx = 0;
	     x += ww;
	     ww = tw;
	  }
	ty = 0;
	y += hh;
	hh = th;
     }
   if (tim != tile)
     {
	imlib_context_set_image(tim);
	imlib_free_image();
	imlib_context_set_image(im);
     }
}

static              Pixmap
BackgroundCreatePixmap(Window win, unsigned int w, unsigned int h,
		       unsigned int depth)
{
   Pixmap              pmap;

   /*
    * Stupid hack to avoid that a new root pixmap has the same ID as the now
    * invalid one from a previous session.
    */
   pmap = ECreatePixmap(win, w, h, depth);
   if (win == VRoot.win && pmap == Mode.hints.old_root_pmap)
     {
	EFreePixmap(pmap);
	pmap = ECreatePixmap(win, w, h, depth);
	Mode.hints.old_root_pmap = None;
     }
   return pmap;
}

Pixmap
BackgroundApply(Background * bg, Drawable draw,
		unsigned int rw, unsigned int rh, int is_win)
{
   Pixmap              pmap;
   GC                  gc;
   int                 rt;
   int                 x, y;
   unsigned int        w, h, ww, hh;
   char                hasbg, hasfg;
   Imlib_Image        *im;

#if ENABLE_COLOR_MODIFIERS
   ColorModifierClass *cm;
#endif

   if (bg->bg.file && !bg->bg.im)
     {
	if (!bg->bg.real_file)
	   bg->bg.real_file = ThemeFileFind(bg->bg.file, 0);
	bg->bg.im = ELoadImage(bg->bg.real_file);
     }

   if (bg->top.file && !bg->top.im)
     {
	if (!bg->top.real_file)
	   bg->top.real_file = ThemeFileFind(bg->top.file, 0);
	bg->top.im = ELoadImage(bg->top.real_file);
     }

#if ENABLE_COLOR_MODIFIERS
   cm = bg->cmclass;
   if (cm)
      cm->ref_count--;
   else
      cm = FindItem("BACKGROUND", 0, LIST_FINDBY_NAME, LIST_TYPE_COLORMODIFIER);

   if (cm)
     {
	cm->ref_count++;
#if 0				/* To be implemented? */
	if (bg->top.im)
	  {
	     Imlib_set_image_red_curve(pImlib_Context, bg->top.im, cm->red.map);
	     Imlib_set_image_green_curve(pImlib_Context, bg->top.im,
					 cm->green.map);
	     Imlib_set_image_blue_curve(pImlib_Context, bg->top.im,
					cm->blue.map);
	  }
	if (bg->bg.im)
	  {
	     Imlib_set_image_red_curve(pImlib_Context, bg->bg.im, cm->red.map);
	     Imlib_set_image_green_curve(pImlib_Context, bg->bg.im,
					 cm->green.map);
	     Imlib_set_image_blue_curve(pImlib_Context, bg->bg.im,
					cm->blue.map);
	  }
#endif
     }
#endif

   hasbg = bg->bg.im != NULL;
   hasfg = bg->top.im != NULL;

   if (!hasbg && !hasfg)
     {
	/* Solid color only */
	EAllocColor(&bg->bg_solid);

	if (is_win)
	  {
	     ESetWindowBackground(draw, bg->bg_solid.pixel);
	  }
	else
	  {
	     gc = ECreateGC(draw, 0, NULL);
	     XSetClipMask(disp, gc, 0);
	     XSetFillStyle(disp, gc, FillSolid);
	     XSetForeground(disp, gc, bg->bg_solid.pixel);
	     XFillRectangle(disp, draw, gc, 0, 0, rw, rh);
	     EFreeGC(gc);
	  }
	return None;
     }

   /* Has either bg or fg image */

   rt = imlib_context_get_dither();
   if (Conf.backgrounds.hiquality)
     {
	imlib_context_set_dither(1);
#if 0				/* ??? */
	imlib_context_set_anti_alias(1);
#endif
     }

   w = h = x = y = 0;

   if (hasbg)
     {
	imlib_context_set_image(bg->bg.im);

	BgFindImageSize(&(bg->bg), rw, rh, &w, &h);
	x = ((rw - w) * bg->bg.xjust) >> 10;
	y = ((rh - h) * bg->bg.yjust) >> 10;
     }

   if (is_win && hasbg && !hasfg && x == 0 && y == 0 &&
       ((w == rw && h == rh) || (bg->bg_tile && !TransparencyEnabled())))
     {
	/* Window, no fg, no offset, and scale to 100%, or tiled, no trans */
	pmap = BackgroundCreatePixmap(draw, w, h, VRoot.depth);
	imlib_context_set_drawable(pmap);
	imlib_render_image_on_drawable_at_size(0, 0, w, h);

	if (x == 0 && y == 0)
	  {
	     ESetWindowBackgroundPixmap(draw, pmap);
	  }
	else
	  {
	     gc = ECreateGC(draw, 0, NULL);
	     XSetTile(disp, gc, pmap);
	     XSetTSOrigin(disp, gc, x, y);
	     XSetFillStyle(disp, gc, FillTiled);
	     XFillRectangle(disp, draw, gc, 0, 0, rw, rh);
	     EFreeGC(gc);
	  }
	goto done;
     }

   /* The rest that require some more work */
   if (is_win)
      pmap = BackgroundCreatePixmap(draw, rw, rh, VRoot.depth);
   else
      pmap = draw;
   imlib_context_set_drawable(pmap);

   if (hasbg && x == 0 && y == 0 && w == rw && h == rh)
     {
	im = bg->bg.im;
	imlib_context_set_image(im);
     }
   else
     {
	/* Create full size image */
	im = imlib_create_image(rw, rh);
	imlib_context_set_image(im);
	imlib_image_set_has_alpha(0);
	if (!hasbg || !bg->bg_tile)
	  {
	     /* Fill solid */
	     imlib_context_set_blend(0);
	     imlib_context_set_color(bg->bg_solid.red, bg->bg_solid.green,
				     bg->bg_solid.blue, 0);
	     imlib_image_fill_rectangle(0, 0, rw, rh);
	  }
	if (hasbg)
	  {
	     if (bg->bg_tile)
	       {
		  e16_tile_image_onto_image(bg->bg.im, 0, w, h,
					    0, 0, rw, rh, x, y);
	       }
	     else
	       {
		  imlib_context_set_image(bg->bg.im);
		  ww = imlib_image_get_width();
		  hh = imlib_image_get_height();
		  imlib_context_set_image(im);
		  imlib_blend_image_onto_image(bg->bg.im, 1, 0, 0, ww, hh,
					       x, y, w, h);
	       }
	  }
     }

   if (hasfg)
     {
	imlib_context_set_image(bg->top.im);

	BgFindImageSize(&(bg->top), rw, rh, &w, &h);
	x = ((rw - w) * bg->top.xjust) >> 10;
	y = ((rh - h) * bg->top.yjust) >> 10;

	ww = imlib_image_get_width();
	hh = imlib_image_get_height();
	imlib_context_set_image(im);
	imlib_blend_image_onto_image(bg->top.im, 0, 0, 0, ww, hh, x, y, w, h);
     }

   imlib_render_image_on_drawable_at_size(0, 0, rw, rh);
   if (im != bg->bg.im)
      imlib_free_image();

 done:
   if (!bg->keepim)
      BackgroundImagesFree(bg, 0);

   imlib_context_set_dither(rt);

   return pmap;
}

/*
 * Apply a background to window/pixmap.
 * The (scaled) BG pixmap is stored in bg->pmap.
 */
void
BackgroundSet(Background * bg, Window win, unsigned int w, unsigned int h)
{
   Pixmap              pmap;

   if (bg->pmap)
      pmap = bg->pmap;
   else
      pmap = BackgroundApply(bg, win, w, h, 1);

   bg->pmap = pmap;
   if (pmap != None)
      ESetWindowBackgroundPixmap(win, pmap);
   else
      ESetWindowBackground(win, bg->bg_solid.pixel);
   EClearWindow(win);
}

static void
BackgroundApply2(Background * bg, Window win)
{
   Window              rr;
   int                 x, y;
   unsigned int        w, h, bw, depth;

   if (!XGetGeometry(disp, win, &rr, &x, &y, &w, &h, &bw, &depth))
      return;

   BackgroundApply(bg, win, w, h, 1);
}

Background         *
BrackgroundCreateFromImage(const char *bgid, const char *file,
			   char *thumb, int thlen)
{
   Background         *bg;
   Imlib_Image        *im, *im2;
   XColor              xclr;
   char                tile = 1, keep_asp = 0;
   int                 width, height;
   int                 scalex = 0, scaley = 0;
   int                 scr_asp, im_asp;
   int                 w2, h2;
   int                 maxw = 48, maxh = 48;
   int                 justx = 512, justy = 512;

   bg = FindItem(bgid, 0, LIST_FINDBY_NAME, LIST_TYPE_BACKGROUND);

   if (thumb)
     {
	Esnprintf(thumb, thlen, "%s/cached/img/%s.png", EDirUserCache(), bgid);
	if (bg && exists(thumb) && moddate(thumb) > moddate(file))
	   return bg;
	/* The thumbnail is gone or outdated - regererate */
     }
   else
     {
	if (bg)
	   return bg;
     }

   im = imlib_load_image(file);
   if (!im)
      return NULL;

   imlib_context_set_image(im);
   width = imlib_image_get_width();
   height = imlib_image_get_height();

   if (thumb)
     {
	h2 = maxh;
	w2 = (width * h2) / height;
	if (w2 > maxw)
	  {
	     w2 = maxw;
	     h2 = (height * w2) / width;
	  }
	im2 = imlib_create_cropped_scaled_image(0, 0, width, height, w2, h2);
	imlib_free_image_and_decache();
	imlib_context_set_image(im2);
	imlib_image_set_format("png");
	imlib_save_image(thumb);
	imlib_free_image_and_decache();
     }

   /* Quit if the background itself already exists */
   if (bg)
      return bg;

   scr_asp = (VRoot.w << 16) / VRoot.h;
   im_asp = (width << 16) / height;
   if (width == height)
     {
	justx = 0;
	justy = 0;
	scalex = 0;
	scaley = 0;
	tile = 1;
	keep_asp = 0;
     }
   else if ((!(IN_RANGE(scr_asp, im_asp, 16000)))
	    && ((width < 480) && (height < 360)))
     {
	justx = 0;
	justy = 0;
	scalex = 0;
	scaley = 0;
	tile = 1;
	keep_asp = 0;
     }
   else if (IN_RANGE(scr_asp, im_asp, 16000))
     {
	justx = 0;
	justy = 0;
	scalex = 1024;
	scaley = 1024;
	tile = 0;
	keep_asp = 0;
     }
   else if (im_asp > scr_asp)
     {
	justx = 512;
	justy = 512;
	scalex = 1024;
	scaley = 0;
	tile = 0;
	keep_asp = 1;
     }
   else
     {
	justx = 512;
	justy = 512;
	scalex = 0;
	scaley = 1024;
	tile = 0;
	keep_asp = 1;
     }

   ESetColor(&xclr, 0, 0, 0);

   bg = BackgroundCreate(bgid, &xclr, file, tile,
			 keep_asp, justx, justy,
			 scalex, scaley, NULL, 0, 0, 0, 0, 0);

   return bg;
}

void
BackgroundIncRefcount(Background * bg)
{
   if (bg == NULL)
      return;
   bg->ref_count++;
}

void
BackgroundDecRefcount(Background * bg)
{
   if (bg == NULL)
      return;
   bg->ref_count--;
   if (bg->ref_count <= 0)
      bg->last_viewed = 0;	/* Clean out asap */
}

void
BackgroundTouch(Background * bg)
{
   if (bg == NULL)
      return;
   bg->last_viewed = time(NULL);
}

const char         *
BackgroundGetName(const Background * bg)
{
   return bg->name;
}

static const char  *
BackgroundGetBgFile(const Background * bg)
{
   return bg->bg.file;
}

static const char  *
BackgroundGetFgFile(const Background * bg)
{
   return bg->top.file;
}

int
BackgroundGetColor(const Background * bg)
{
   return (bg->pmap) ? 0 : bg->bg_solid.pixel;
}

Pixmap
BackgroundGetPixmap(const Background * bg)
{
   return (bg) ? bg->pmap : None;
}

static              time_t
BackgroundGetTimestamp(const Background * bg)
{
   return bg->last_viewed;
}

static Imlib_Image *
BackgroundCacheMini(Background * bg, int keep, int nuke)
{
   char                s[4096];
   Imlib_Image        *im;
   Pixmap              pmap;

   Esnprintf(s, sizeof(s), "%s/cached/bgsel/%s.png", EDirUserCache(),
	     BackgroundGetName(bg));

   im = ELoadImage(s);
   if (im)
     {
	imlib_context_set_image(im);
	if (nuke)
	   imlib_free_image_and_decache();
	else
	   goto done;
     }

   /* Create new cached bg mini image */
   pmap = ECreatePixmap(VRoot.win, 64, 48, VRoot.depth);
   BackgroundApply(bg, pmap, 64, 48, 0);
   imlib_context_set_drawable(pmap);
   im = imlib_create_image_from_drawable(0, 0, 0, 64, 48, 0);
   imlib_context_set_image(im);
   imlib_image_set_format("png");
   imlib_save_image(s);
   EFreePixmap(pmap);

 done:
   if (keep)
      return im;
   imlib_free_image();
   return NULL;
}

#define S(str) ((str) ? str : "(null)")
static void
BackgroundGetInfoString1(const Background * bg, char *buf, int len)
{
   int                 r, g, b;

   EGetColor(&(bg->bg_solid), &r, &g, &b);
   Esnprintf(buf, len,
	     "%s ref_count %u keepim %u\n"
	     " bg.solid\t %i %i %i \n"
	     " bg.file\t %s\n"
	     " top.file\t %s \n"
	     " bg.tile\t %i \n"
	     " bg.keep_aspect\t %i \ttop.keep_aspect\t %i \n"
	     " bg.xjust\t %i \ttop.xjust\t %i \n"
	     " bg.yjust\t %i \ttop.yjust\t %i \n"
	     " bg.xperc\t %i \ttop.xperc\t %i \n"
	     " bg.yperc\t %i \ttop.yperc\t %i \n", bg->name,
	     bg->ref_count, bg->keepim, r, g, b,
	     bg->bg.file, bg->top.file, bg->bg_tile,
	     bg->bg.keep_aspect, bg->top.keep_aspect,
	     bg->bg.xjust, bg->top.xjust, bg->bg.yjust,
	     bg->top.yjust, bg->bg.xperc, bg->top.xperc,
	     bg->bg.yperc, bg->top.yperc);
}

static void
BackgroundGetInfoString2(const Background * bg, char *buf, int len)
{
   int                 r, g, b;

   EGetColor(&(bg->bg_solid), &r, &g, &b);
   Esnprintf(buf, len,
	     "%s %i %i %i %s %i %i %i %i %i %i %s %i %i %i %i %i",
	     bg->name, r, g, b, S(bg->bg.file), bg->bg_tile,
	     bg->bg.keep_aspect, bg->bg.xjust, bg->bg.yjust,
	     bg->bg.xperc, bg->bg.yperc, S(bg->top.file),
	     bg->top.keep_aspect, bg->top.xjust, bg->top.yjust,
	     bg->top.xperc, bg->top.yperc);
}

/*
 * Config load/save
 */
#include "conf.h"

int
BackgroundsConfigLoad(FILE * fs)
{
   int                 err = 0;
   Background         *bg = 0;
   XColor              xclr;
   char                s[FILEPATH_LEN_MAX];
   char                s2[FILEPATH_LEN_MAX];
   int                 ii1;
   int                 r, g, b;
   int                 i1 = 0, i2 = 0, i3 = 0, i4 = 0, i5 = 0, i6 = 0;
   int                 j1 = 0, j2 = 0, j3 = 0, j4 = 0, j5 = 0;
   char               *bg1 = 0;
   char               *bg2 = 0;
   char               *name = 0;
   char                ignore = 0;
   int                 fields;
   unsigned int        desk;

#if ENABLE_COLOR_MODIFIERS
   ColorModifierClass *cm = NULL;
#endif

   ESetColor(&xclr, 0, 0, 0);

   while (GetLine(s, sizeof(s), fs))
     {
	s2[0] = 0;
	ii1 = CONFIG_INVALID;
	fields = sscanf(s, "%i %4000s", &ii1, s2);

	if (fields < 1)
	  {
	     ii1 = CONFIG_INVALID;
	  }
	else if (ii1 == CONFIG_CLOSE)
	  {
	     if (fields != 1)
	       {
		  RecoverUserConfig();
		  Alert(_("CONFIG: ignoring extra data in \"%s\"\n"), s);
	       }
	  }
	else if (ii1 != CONFIG_INVALID)
	  {
	     if (fields != 2)
	       {
		  RecoverUserConfig();
		  Alert(_("CONFIG: missing required data in \"%s\"\n"), s);
		  ii1 = CONFIG_INVALID;
	       }
	  }
	switch (ii1)
	  {
	  case CONFIG_CLOSE:
	     if (!ignore)
	       {
		  if ((!bg) && (name))
		    {
		       char               *tmp;
		       char                ok = 1;

		       /* check first if we can actually find the files */
		       if (bg1)
			 {
			    tmp = ThemeFileFind(bg1, 0);
			    if (!tmp)
			       ok = 0;
			    else
			       Efree(tmp);
			 }
		       if (bg2)
			 {
			    tmp = ThemeFileFind(bg2, 0);
			    if (!tmp)
			       ok = 0;
			    else
			       Efree(tmp);
			 }
		       if (ok)
			 {
			    bg = BackgroundCreate(name, &xclr, bg1, i1, i2, i3,
						  i4, i5, i6, bg2, j1, j2, j3,
						  j4, j5);
#if ENABLE_COLOR_MODIFIERS
			    if (cm)
			       BackgroundSetColorMofifier(bg, cm);
#endif
			 }
		    }
	       }
	     goto done;

	  case CONFIG_COLORMOD:
	  case ICLASS_COLORMOD:
#if ENABLE_COLOR_MODIFIERS
	     cm = FindItem(s2, 0, LIST_FINDBY_NAME, LIST_TYPE_COLORMODIFIER);
#endif
	     break;

	  case CONFIG_CLASSNAME:
	  case BG_NAME:
	     if ((bg = FindItem(s2, 0, LIST_FINDBY_NAME, LIST_TYPE_BACKGROUND)))
	       {
		  ignore = 1;
	       }
	     else
	       {
		  if (name)
		     Efree(name);
		  name = Estrdup(s2);
	       }
	     break;

	  case BG_DESKNUM:
	     desk = atoi(s2);
	     if (desk < DesksGetNumber())
	       {
		  if ((DeskGetBackground(DeskGet(desk)) == NULL) ||
		      (Conf.backgrounds.user))
		    {
		       if (!ignore)
			 {
			    if (!bg)
			       bg = BackgroundCreate(name, &xclr, bg1, i1, i2,
						     i3, i4, i5, i6, bg2, j1,
						     j2, j3, j4, j5);
			 }
		       DeskAssignBg(desk, bg);
		    }
	       }
	     break;

	  case BG_RGB:
	     r = g = b = 0;
	     sscanf(s, "%*s %d %d %d", &r, &g, &b);
	     ESetColor(&xclr, r, g, b);
	     if (ignore)
		bg->bg_solid = xclr;
	     break;

	  case BG_BG1:
	     sscanf(s, "%*s %4000s %d %d %d %d %d %d", s2, &i1, &i2,
		    &i3, &i4, &i5, &i6);
	     if (!ignore)
	       {
		  if (bg1)
		     Efree(bg1);
		  bg1 = Estrdup(s2);
	       }
	     else
	       {
		  if (bg->bg.file)
		     Efree(bg->bg.file);
		  if (bg->top.file)
		    {
		       Efree(bg->top.file);
		       bg->top.file = NULL;
		    }
		  bg->bg.file = Estrdup(s2);
		  bg->bg_tile = i1;
		  bg->bg.keep_aspect = i2;
		  bg->bg.xjust = i3;
		  bg->bg.yjust = i4;
		  bg->bg.xperc = i5;
		  bg->bg.yperc = i6;
	       }
	     break;

	  case BG_BG2:
	     sscanf(s, "%*s %4000s %d %d %d %d %d", s2, &j1, &j2, &j3,
		    &j4, &j5);
	     if (!ignore)
	       {
		  if (bg2)
		     Efree(bg2);
		  bg2 = Estrdup(s2);
	       }
	     else
	       {
		  bg->top.file = Estrdup(s2);
		  bg->top.keep_aspect = j1;
		  bg->top.xjust = j2;
		  bg->top.yjust = j3;
		  bg->top.xperc = j4;
		  bg->top.yperc = j5;
	       }
	     break;

	  default:
	     break;
	  }
     }
   err = -1;

 done:
   if (name)
      Efree(name);
   if (bg1)
      Efree(bg1);
   if (bg2)
      Efree(bg2);

   return err;
}

static void
BackgroundsConfigLoadUser(void)
{
   char                s[4096];

   Esnprintf(s, sizeof(s), "%s.backgrounds", EGetSavePrefix());
   ConfigFileLoad(s, NULL, ConfigFileRead, 0);
}

static void
BackgroundsConfigSave(void)
{
   char                s[FILEPATH_LEN_MAX], st[FILEPATH_LEN_MAX];
   FILE               *fs;
   int                 i, num;
   Background        **bglist;
   unsigned int        j;
   int                 r, g, b;

   bglist = (Background **) ListItemType(&num, LIST_TYPE_BACKGROUND);
   if (num <= 0)
      return;

   Etmp(st);
   fs = fopen(st, "w");
   if (!fs)
      return;

   for (i = num - 1; i >= 0; i--)
     {
	fprintf(fs, "5 999\n");

	fprintf(fs, "100 %s\n", bglist[i]->name);
	EGetColor(&(bglist[i]->bg_solid), &r, &g, &b);
	fprintf(fs, "560 %d %d %d\n", r, g, b);

	if ((bglist[i]->bg.file) && (!bglist[i]->bg.real_file))
	   bglist[i]->bg.real_file = ThemeFileFind(bglist[i]->bg.file, 0);

	if ((bglist[i]->top.file) && (!bglist[i]->top.real_file))
	   bglist[i]->top.real_file = ThemeFileFind(bglist[i]->top.file, 0);

	if ((bglist[i]->bg.file) && (bglist[i]->bg.real_file))
	  {
	     fprintf(fs, "561 %s %d %d %d %d %d %d\n",
		     bglist[i]->bg.real_file, bglist[i]->bg_tile,
		     bglist[i]->bg.keep_aspect, bglist[i]->bg.xjust,
		     bglist[i]->bg.yjust, bglist[i]->bg.xperc,
		     bglist[i]->bg.yperc);
	  }
	else if (bglist[i]->bg.file)
	  {
	     fprintf(fs, "561 %s %d %d %d %d %d %d\n",
		     bglist[i]->bg.file, bglist[i]->bg_tile,
		     bglist[i]->bg.keep_aspect, bglist[i]->bg.xjust,
		     bglist[i]->bg.yjust, bglist[i]->bg.xperc,
		     bglist[i]->bg.yperc);
	  }

	if ((bglist[i]->top.file) && (bglist[i]->top.real_file))
	  {
	     fprintf(fs, "562 %s %d %d %d %d %d\n",
		     bglist[i]->top.real_file,
		     bglist[i]->top.keep_aspect, bglist[i]->top.xjust,
		     bglist[i]->top.yjust, bglist[i]->top.xperc,
		     bglist[i]->top.yperc);
	  }
	else if (bglist[i]->top.file)
	  {
	     fprintf(fs, "562 %s %d %d %d %d %d\n",
		     bglist[i]->top.file, bglist[i]->top.keep_aspect,
		     bglist[i]->top.xjust, bglist[i]->top.yjust,
		     bglist[i]->top.xperc, bglist[i]->top.yperc);
	  }

#if ENABLE_COLOR_MODIFIERS
	if (bglist[i]->cmclass)
	  {
	     fprintf(fs, "370 %s\n", bglist[i]->cmclass->name);
	  }
#endif

	for (j = 0; j < (DesksGetNumber()); j++)
	  {
	     Desk               *dsk = DeskGet(j);

	     if ((!strcmp(bglist[i]->name, "NONE"))
		 && (!DeskGetBackground(dsk)))
		fprintf(fs, "564 %d\n", j);
	     if (DeskGetBackground(dsk) == bglist[i])
		fprintf(fs, "564 %d\n", j);
	  }

	fprintf(fs, "1000\n");
     }

   fclose(fs);

   Esnprintf(s, sizeof(s), "%s.backgrounds", EGetSavePrefix());
   E_mv(st, s);

   Efree(bglist);
}

/*
 * Backgrounds module
 */

static void
BackgroundsAccounting(void)
{
   time_t              now;
   int                 i, num;
   unsigned int        j;
   Background        **lst;
   Window              win;
   Desk               *dsk;

   now = time(NULL);

   for (j = 0; j < DesksGetNumber(); j++)
     {
	dsk = DeskGet(j);
	if ((DeskGetBackground(dsk)) && (DeskIsViewable(dsk)))
	   BackgroundTouch(DeskGetBackground(dsk));
     }

   lst = (Background **) ListItemType(&num, LIST_TYPE_BACKGROUND);
   for (i = 0; i < num; i++)
     {
	/* Skip if no pixmap or not timed out */
	if ((BackgroundGetPixmap(lst[i]) == 0) ||
	    ((now - BackgroundGetTimestamp(lst[i])) <=
	     Conf.backgrounds.timeout))
	   continue;

	/* Skip if associated with any viewable desktop */
	for (j = 0; j < DesksGetNumber(); j++)
	  {
	     dsk = DeskGet(j);
	     if (lst[i] == DeskGetBackground(dsk) && DeskIsViewable(dsk))
		goto next;
	  }

	for (j = 0; j < DesksGetNumber(); j++)
	  {
	     dsk = DeskGet(j);
	     if (lst[i] != DeskGetBackground(dsk) || DeskIsViewable(dsk))
		continue;

	     /* Unviewable desktop - update the virtual root hints */
	     win = EoGetWin(dsk);
	     if (!Conf.hints.set_xroot_info_on_root_window)
		HintsSetRootInfo(win, 0, 0);
	     ESetWindowBackground(win, 0);
	     EClearWindow(win);
	  }

	BackgroundPixmapFree(lst[i]);

      next:
	;
     }
   if (lst)
      Efree(lst);
}

static void
BackgroundsTimeout(int val __UNUSED__, void *data __UNUSED__)
{
   BackgroundsAccounting();
/* RemoveTimerEvent("BACKGROUND_ACCOUNTING_TIMEOUT"); */
   DoIn("BACKGROUND_ACCOUNTING_TIMEOUT", 30.0, BackgroundsTimeout, 0, NULL);
}

static void
BackgroundsSighan(int sig, void *prm __UNUSED__)
{
   switch (sig)
     {
     case ESIGNAL_INIT:
	EDirMake(EDirUserCache(), "cached/bgsel");
	EDirMake(EDirUserCache(), "cached/img");
	/* create a fallback background in case no background is found */
	BackgroundCreate("NONE", NULL, NULL, 0, 0, 0, 0, 0, 0, NULL, 0, 0, 0, 0,
			 0);
	break;

     case ESIGNAL_CONFIGURE:
	BackgroundsConfigLoadUser();
	break;

     case ESIGNAL_START:
	DoIn("BACKGROUND_ACCOUNTING_TIMEOUT", 30.0, BackgroundsTimeout, 0,
	     NULL);
	break;

     case ESIGNAL_EXIT:
	if (Mode.wm.save_ok)
	   BackgroundsConfigSave();
	break;
     }
}

/*
 * Configuration dialog
 */
static Dialog      *bg_sel_dialog;
static DItem       *bg_sel;
static DItem       *bg_sel_slider;
static DItem       *bg_mini_disp;
static DItem       *bg_filename;
static DItem       *tmp_w[10];

static Background  *tmp_bg;	/* The background being configured */
static Pixmap       tmp_bg_mini_pixmap = None;
static int          tmp_bg_sel_sliderval = 0;
static int          tmp_bg_sel_sliderval_old = -1;
static int          tmp_bg_r;
static int          tmp_bg_g;
static int          tmp_bg_b;
static char         tmp_bg_image;
static char         tmp_bg_tile;
static char         tmp_bg_keep_aspect;
static int          tmp_bg_xjust;
static int          tmp_bg_yjust;
static int          tmp_bg_xperc;
static int          tmp_bg_yperc;
static char         tmp_hiq;
static char         tmp_userbg;
static int          tmp_bg_timeout;

static void         BG_RedrawView(void);

static void
CB_ConfigureBG(Dialog * d __UNUSED__, int val, void *data __UNUSED__)
{
   unsigned int        i;

   if (val < 0)
     {
	BackgroundImagesKeep(tmp_bg, 0);
	tmp_bg = NULL;
	if (tmp_bg_mini_pixmap != None)
	   EFreePixmap(tmp_bg_mini_pixmap);
	tmp_bg_mini_pixmap = None;
	return;
     }

   if (val < 2)
     {
	Conf.backgrounds.timeout = tmp_bg_timeout;
	Conf.backgrounds.hiquality = tmp_hiq;
	Conf.backgrounds.user = tmp_userbg;

	ESetColor(&(tmp_bg->bg_solid), tmp_bg_r, tmp_bg_g, tmp_bg_b);
	tmp_bg->bg_tile = tmp_bg_tile;
	tmp_bg->bg.keep_aspect = tmp_bg_keep_aspect;
	tmp_bg->bg.xjust = tmp_bg_xjust;
	tmp_bg->bg.yjust = 1024 - tmp_bg_yjust;
	tmp_bg->bg.xperc = tmp_bg_xperc;
	tmp_bg->bg.yperc = 1024 - tmp_bg_yperc;

	for (i = 0; i < DesksGetNumber(); i++)
	  {
	     Desk               *dsk = DeskGet(i);

	     if (DeskGetBackground(dsk) == tmp_bg)
		DeskSetBg(dsk, tmp_bg, 1);
	  }

	BackgroundCacheMini(tmp_bg, 0, 1);
	BG_RedrawView();

	if (!tmp_bg_image)
	   BackgroundImagesRemove(tmp_bg);
     }

   autosave();
}

/* Draw the background preview image */
static void
CB_DesktopMiniDisplayRedraw(Dialog * d __UNUSED__, int val, void *data)
{
   Background         *bg;
   Pixmap              pmap;
   int                 w, h;
   DItem              *di;
   Window              win;
   XColor              xclr;
   const char         *fbg, *ffg;

   if (!tmp_bg)
      return;

   di = (DItem *) data;
   win = DialogItemAreaGetWindow(di);
   DialogItemAreaGetSize(di, &w, &h);

   if (tmp_bg_mini_pixmap == None)
      tmp_bg_mini_pixmap = ECreatePixmap(win, w, h, VRoot.depth);
   pmap = tmp_bg_mini_pixmap;

   if (val == 1)
     {
	ESetWindowBackgroundPixmap(win, pmap);
	BackgroundApply(tmp_bg, pmap, w, h, 0);
     }
   else
     {
	fbg = (tmp_bg_image) ? BackgroundGetBgFile(tmp_bg) : NULL;
	ffg = (tmp_bg_image) ? BackgroundGetFgFile(tmp_bg) : NULL;
	ESetColor(&xclr, tmp_bg_r, tmp_bg_g, tmp_bg_b);
	bg = BackgroundCreate("TEMP", &xclr, fbg, tmp_bg_tile,
			      tmp_bg_keep_aspect, tmp_bg_xjust,
			      1024 - tmp_bg_yjust, tmp_bg_xperc,
			      1024 - tmp_bg_yperc, ffg,
			      tmp_bg->top.keep_aspect, tmp_bg->top.xjust,
			      tmp_bg->top.yjust, tmp_bg->top.xperc,
			      tmp_bg->top.yperc);

	BackgroundApply(bg, pmap, w, h, 0);
	BackgroundDestroy(bg);
     }
   EClearWindow(win);
}

/* Update tmp vars according to the current tmp_bg */
static void
BG_GetValues(void)
{
   tmp_bg_image = (tmp_bg->bg.file) ? 1 : 0;

   EGetColor(&(tmp_bg->bg_solid), &tmp_bg_r, &tmp_bg_g, &tmp_bg_b);
   tmp_bg_tile = tmp_bg->bg_tile;
   tmp_bg_keep_aspect = tmp_bg->bg.keep_aspect;
   tmp_bg_xjust = tmp_bg->bg.xjust;
   tmp_bg_yjust = 1024 - tmp_bg->bg.yjust;
   tmp_bg_xperc = tmp_bg->bg.xperc;
   tmp_bg_yperc = 1024 - tmp_bg->bg.yperc;
}

static void
BG_DialogSetFileName(DItem * di)
{
   char               *stmp;
   char                s[1024];

   if (BackgroundGetBgFile(tmp_bg))
      stmp = fullfileof(BackgroundGetBgFile(tmp_bg));
   else
      stmp = Estrdup(_("-NONE-"));
   Esnprintf(s, sizeof(s),
	     _("Background definition information:\nName: %s\nFile: %s\n"),
	     BackgroundGetName(tmp_bg), stmp);
   Efree(stmp);
   DialogItemSetText(di, s);
}

static void
BgDialogSetNewCurrent(Background * bg)
{
   int                 i;

   if (tmp_bg && tmp_bg != bg)
      BackgroundImagesKeep(tmp_bg, 0);
   tmp_bg = bg;
   BackgroundImagesKeep(tmp_bg, 1);

   /* Fetch new BG values */
   BG_GetValues();

   /* Update dialog items */
   BG_DialogSetFileName(bg_filename);
   DialogDrawItems(bg_sel_dialog, bg_filename, 0, 0, 99999, 99999);

   DialogItemSliderSetVal(tmp_w[0], tmp_bg_r);
   DialogItemCheckButtonSetState(tmp_w[1], tmp_bg_image);
   DialogItemSliderSetVal(tmp_w[2], tmp_bg_g);
   DialogItemCheckButtonSetState(tmp_w[3], tmp_bg_keep_aspect);
   DialogItemSliderSetVal(tmp_w[4], tmp_bg_b);
   DialogItemCheckButtonSetState(tmp_w[5], tmp_bg_tile);
   DialogItemSliderSetVal(tmp_w[6], tmp_bg_xjust);
   DialogItemSliderSetVal(tmp_w[7], tmp_bg_yjust);
   DialogItemSliderSetVal(tmp_w[8], tmp_bg_yperc);
   DialogItemSliderSetVal(tmp_w[9], tmp_bg_xperc);

   /* Redraw preview image */
   CB_DesktopMiniDisplayRedraw(NULL, 1, bg_mini_disp);

   /* Redraw scrolling BG list */
   BG_RedrawView();

   for (i = 0; i < 10; i++)
      DialogDrawItems(bg_sel_dialog, tmp_w[i], 0, 0, 99999, 99999);
}

/* Duplicate current (tmp_bg) to new */
static void
CB_ConfigureNewBG(Dialog * d __UNUSED__, int val __UNUSED__,
		  void *data __UNUSED__)
{
   char                s[1024];
   XColor              xclr;
   int                 lower, upper;

   Esnprintf(s, sizeof(s), "__NEWBG_%i", (unsigned)time(NULL));

   ESetColor(&xclr, tmp_bg_r, tmp_bg_g, tmp_bg_b);

   tmp_bg = BackgroundCreate(s, &xclr, tmp_bg->bg.file, tmp_bg_tile,
			     tmp_bg_keep_aspect, tmp_bg_xjust,
			     1024 - tmp_bg_yjust, tmp_bg_xperc,
			     1024 - tmp_bg_yperc, tmp_bg->top.file,
			     tmp_bg->top.keep_aspect, tmp_bg->top.xjust,
			     tmp_bg->top.yjust, tmp_bg->top.xperc,
			     tmp_bg->top.yperc);

   DialogItemSliderGetBounds(bg_sel_slider, &lower, &upper);
   upper += 4;
   DialogItemSliderSetBounds(bg_sel_slider, lower, upper);

   DialogItemSliderSetVal(bg_sel_slider, 0);
   DialogDrawItems(bg_sel_dialog, bg_sel_slider, 0, 0, 99999, 99999);

   DeskSetBg(DesksGetCurrent(), tmp_bg, 0);

   BG_RedrawView();

   autosave();
}

static void
CB_ConfigureDelBG(Dialog * d __UNUSED__, int val, void *data __UNUSED__)
{
   Background        **bglist, *bg;
   int                 i, num;
   int                 slider, lower, upper;

   bglist = (Background **) ListItemType(&num, LIST_TYPE_BACKGROUND);
   if (!bglist || num <= 1)
      goto done;

   DialogItemSliderGetBounds(bg_sel_slider, &lower, &upper);
   slider = DialogItemSliderGetVal(bg_sel_slider);
   upper -= 4;
   DialogItemSliderSetBounds(bg_sel_slider, lower, upper);
   if (slider > upper)
      DialogItemSliderSetVal(bg_sel_slider, upper);

   bg = NULL;
   for (i = 0; i < num; i++)
     {
	if (bglist[i] == tmp_bg)
	  {
	     if (i < (num - 1))
		bg = bglist[i + 1];
	     else
		bg = bglist[i - 1];
	     break;
	  }
     }

   DeskSetBg(DesksGetCurrent(), bg, 0);
   if (val == 0)
      BackgroundDestroy(tmp_bg);
   else
      BackgroundDelete(tmp_bg);
   tmp_bg = NULL;

   BgDialogSetNewCurrent(bg);

   autosave();

 done:
   if (bglist)
      Efree(bglist);
}

/* Move current background to first position in list */
static void
CB_ConfigureFrontBG(Dialog * d __UNUSED__, int val __UNUSED__,
		    void *data __UNUSED__)
{
   MoveItemToListTop(tmp_bg, LIST_TYPE_BACKGROUND);
   BG_RedrawView();
   autosave();
}

static int          tmp_bg_selected = -1;

/* Draw the scrolling background image window */
static void
BG_RedrawView(void)
{
   int                 num, i;
   Background        **bglist;
   int                 w, h;
   Window              win;
   int                 x;
   Pixmap              pmap;
   GC                  gc;
   ImageClass         *ic_button;

   bglist = (Background **) ListItemType(&num, LIST_TYPE_BACKGROUND);
   if (!bglist)
      return;

   win = DialogItemAreaGetWindow(bg_sel);
   DialogItemAreaGetSize(bg_sel, &w, &h);

   pmap = ECreatePixmap(win, w, h, VRoot.depth);
   gc = ECreateGC(pmap, 0, NULL);

   ic_button = ImageclassFind("DIALOG_BUTTON", 0);

   XSetForeground(disp, gc, BlackPixel(disp, VRoot.scr));
   XFillRectangle(disp, pmap, gc, 0, 0, w, h);
   ESetWindowBackgroundPixmap(win, pmap);

   x = -(num * (64 + 8) - w) * tmp_bg_sel_sliderval / (4 * num);

   for (i = 0; i < num; i++)
     {
	if (((x + 64 + 8) >= 0) && (x < w))
	  {
	     Imlib_Image        *im;

	     if (ic_button)
		ImageclassApplySimple(ic_button, win, pmap,
				      (i == tmp_bg_selected) ?
				      STATE_CLICKED : STATE_NORMAL,
				      x, 0, 64 + 8, 48 + 8);

	     if (!strcmp(BackgroundGetName(bglist[i]), "NONE"))
	       {
		  TextClass          *tc;

		  tc = TextclassFind("DIALOG", 1);
		  if (tc)
		    {
		       int                 tw, th;

		       TextSize(tc, 0, 0, STATE_NORMAL,
				_("No\nBackground"), &tw, &th, 17);
		       TextDraw(tc, pmap, 0, 0, STATE_NORMAL,
				_("No\nBackground"), x + 4,
				4 + ((48 - th) / 2), 64, 48, 17, 512);
		    }
	       }
	     else
	       {
		  im = BackgroundCacheMini(bglist[i], 1, 0);
		  if (im)
		    {
		       imlib_context_set_image(im);
		       imlib_context_set_drawable(pmap);
		       imlib_render_image_on_drawable_at_size(x + 4, 4, 64, 48);
		       imlib_free_image();
		    }
	       }
	  }
	x += (64 + 8);
     }
   EFreeGC(gc);
   EFreePixmap(pmap);
   Efree(bglist);

   EClearWindow(win);
}

static void
CB_BGAreaSlide(Dialog * d __UNUSED__, int val __UNUSED__, void *data __UNUSED__)
{
   if (tmp_bg_sel_sliderval == tmp_bg_sel_sliderval_old)
      return;
   BG_RedrawView();
   tmp_bg_sel_sliderval_old = tmp_bg_sel_sliderval;
}

static void
CB_BGScan(Dialog * d, int val __UNUSED__, void *data __UNUSED__)
{
   int                 slider, lower, upper;

   SoundPlay("SOUND_WAIT");

   DialogItemSliderGetBounds(bg_sel_slider, &lower, &upper);

   for (slider = lower; slider <= upper; slider += 8)
     {
	DialogItemSliderSetVal(bg_sel_slider, slider);
	DialogDrawItems(bg_sel_dialog, bg_sel_slider, 0, 0, 99999, 99999);
	DialogItemCallCallback(d, bg_sel_slider);
     }
}

static void
CB_BGAreaEvent(int val __UNUSED__, void *data)
{
   int                 x, num, w, h;
   Background        **bglist;
   XEvent             *ev = (XEvent *) data;

   DialogItemAreaGetSize(bg_sel, &w, &h);

   switch (ev->type)
     {
     case ButtonPress:
	bglist = (Background **) ListItemType(&num, LIST_TYPE_BACKGROUND);
	if (!bglist)
	   break;

	x = (num * (64 + 8) - w) * tmp_bg_sel_sliderval / (4 * num) +
	   ev->xbutton.x;
	tmp_bg_selected = x / (64 + 8);
	if ((tmp_bg_selected >= 0) && (tmp_bg_selected < num))
	  {
	     BgDialogSetNewCurrent(bglist[tmp_bg_selected]);
	     DeskSetBg(DesksGetCurrent(), tmp_bg, 0);
	     autosave();
	  }
	Efree(bglist);
	break;
     }
}

static void
CB_DesktopTimeout(Dialog * d __UNUSED__, int val __UNUSED__, void *data)
{
   DItem              *di;
   char                s[256];

   di = (DItem *) data;
   Esnprintf(s, sizeof(s), _("Unused backgrounds freed after %2i:%02i:%02i"),
	     tmp_bg_timeout / 3600,
	     (tmp_bg_timeout / 60) - (60 * (tmp_bg_timeout / 3600)),
	     (tmp_bg_timeout) - (60 * (tmp_bg_timeout / 60)));
   DialogItemSetText(di, s);
   DialogDrawItems(bg_sel_dialog, di, 0, 0, 99999, 99999);
}

static void
BGSettingsGoTo(Background * bg)
{
   Background        **bglist;
   int                 i, num;

   if (!FindItem("CONFIGURE_BG", 0, LIST_FINDBY_NAME, LIST_TYPE_DIALOG))
      return;

   bglist = (Background **) ListItemType(&num, LIST_TYPE_BACKGROUND);
   if (!bglist)
      return;

   for (i = 0; i < num; i++)
     {
	if (bglist[i] == bg)
	  {
	     DialogItemSliderSetVal(bg_sel_slider, 4 * i);
	     DialogDrawItems(bg_sel_dialog, bg_sel_slider, 0, 0, 99999, 99999);
	     DialogItemCallCallback(NULL, bg_sel_slider);
	     tmp_bg_selected = i;
	     BgDialogSetNewCurrent(bglist[tmp_bg_selected]);
	     tmp_bg_selected = -1;
	     break;
	  }
     }
   Efree(bglist);
}

static void
CB_BGPrev(Dialog * d __UNUSED__, int val __UNUSED__, void *data __UNUSED__)
{
   Background        **bglist;
   int                 i, num;

   bglist = (Background **) ListItemType(&num, LIST_TYPE_BACKGROUND);
   for (i = 0; i < num; i++)
     {
	if ((bglist[i] == tmp_bg) && (i > 0))
	  {
	     BGSettingsGoTo(bglist[i - 1]);
	     DeskSetBg(DesksGetCurrent(), bglist[i - 1], 0);
	     break;
	  }
     }
   if (bglist)
      Efree(bglist);
}

static void
CB_BGNext(Dialog * d __UNUSED__, int val __UNUSED__, void *data __UNUSED__)
{
   Background        **bglist;
   int                 i, num;

   bglist = (Background **) ListItemType(&num, LIST_TYPE_BACKGROUND);
   for (i = 0; i < num; i++)
     {
	if ((bglist[i] == tmp_bg) && (i < (num - 1)))
	  {
	     BGSettingsGoTo(bglist[i + 1]);
	     DeskSetBg(DesksGetCurrent(), bglist[i + 1], 0);
	     break;
	  }
     }
   if (bglist)
      Efree(bglist);
}

static int
BG_SortFileCompare(Background * bg1, Background * bg2)
{
   const char         *name1, *name2;

   /* return < 0 is b1 <  b2 */
   /* return > 0 is b1 >  b2 */
   /* return   0 is b1 == b2 */

   name1 = BackgroundGetBgFile(bg1);
   name2 = BackgroundGetBgFile(bg2);
   if (name1 && name2)
      return strcmp(name1, name2);

   name1 = BackgroundGetFgFile(bg1);
   name2 = BackgroundGetFgFile(bg2);
   if (name1 && name2)
      return strcmp(name1, name2);

   return 0;
}

static void
CB_BGSortFile(Dialog * d __UNUSED__, int val __UNUSED__, void *data __UNUSED__)
{
   Background        **bglist;
   int                 i, num;

   bglist = (Background **) ListItemType(&num, LIST_TYPE_BACKGROUND);
   if (!bglist)
      return;

   /* remove them all from the list */
   for (i = 0; i < num; i++)
      RemoveItemByPtr(bglist[i], LIST_TYPE_BACKGROUND);
   Quicksort((void **)bglist, 0, num - 1,
	     (int (*)(void *d1, void *d2))BG_SortFileCompare);
   for (i = 0; i < num; i++)
     {
	Background         *bg;

	bg = bglist[i];
	AddItem(bg, BackgroundGetName(bg), 0, LIST_TYPE_BACKGROUND);
     }
   Efree(bglist);

   BGSettingsGoTo(tmp_bg);

   autosave();
}

static void
CB_BGSortAttrib(Dialog * d __UNUSED__, int val __UNUSED__,
		void *data __UNUSED__)
{
   Background        **bglist;
   int                 i, num;

   bglist = (Background **) ListItemType(&num, LIST_TYPE_BACKGROUND);
   if (!bglist)
      return;

   /* remove them all from the list */
   for (i = 0; i < num; i++)
      RemoveItemByPtr(bglist[i], LIST_TYPE_BACKGROUND);
   for (i = 0; i < num; i++)
     {
	Background         *bg;

	bg = bglist[i];
	if ((bg) && (bg->bg_tile) && (bg->bg.xperc == 0) && (bg->bg.yperc == 0))
	  {
	     AddItem(bg, bg->name, 0, LIST_TYPE_BACKGROUND);
	     bglist[i] = NULL;
	  }
     }
   for (i = 0; i < num; i++)
     {
	Background         *bg;

	bg = bglist[i];
	if (bg)
	  {
	     AddItem(bg, bg->name, 0, LIST_TYPE_BACKGROUND);
	     bglist[i] = NULL;
	  }
     }
   Efree(bglist);

   BGSettingsGoTo(tmp_bg);

   autosave();
}

static void
CB_BGSortContent(Dialog * d __UNUSED__, int val __UNUSED__,
		 void *data __UNUSED__)
{
   Background        **bglist;
   int                 i, num;

   bglist = (Background **) ListItemType(&num, LIST_TYPE_BACKGROUND);
   if (bglist)
     {
	/* remove them all from the list */
	for (i = 0; i < num; i++)
	   RemoveItemByPtr(bglist[i], LIST_TYPE_BACKGROUND);
	for (i = 0; i < num; i++)
	  {
	     Background         *bg;

	     bg = bglist[i];
	     AddItem(bg, BackgroundGetName(bg), 0, LIST_TYPE_BACKGROUND);
	  }
	Efree(bglist);
     }
   autosave();
}

static void
SettingsBackground(Background * bg)
{
   Dialog             *d;
   DItem              *table, *di, *table2, *area, *slider, *slider2, *label;
   DItem              *w1, *w2, *w3, *w4, *w5, *w6;
   int                 num;
   char                s[1024];

   if ((d = FindItem("CONFIGURE_BG", 0, LIST_FINDBY_NAME, LIST_TYPE_DIALOG)))
     {
	SoundPlay("SOUND_SETTINGS_ACTIVE");
	ShowDialog(d);
	return;
     }
   SoundPlay("SOUND_SETTINGS_BG");

   if ((!bg) || ((bg) && (!strcmp(BackgroundGetName(bg), "NONE"))))
     {
	Esnprintf(s, sizeof(s), "__NEWBG_%i", (unsigned)time(NULL));
	bg = BackgroundCreate(s, NULL, NULL, 1, 1, 0, 0, 0, 0, NULL, 1,
			      512, 512, 0, 0);
     }
   tmp_bg = bg;

   BG_GetValues();

   tmp_hiq = Conf.backgrounds.hiquality;
   tmp_userbg = Conf.backgrounds.user;
   tmp_bg_timeout = Conf.backgrounds.timeout;

   d = bg_sel_dialog = DialogCreate("CONFIGURE_BG");
   DialogSetTitle(d, _("Desktop Background Settings"));

   table = DialogInitItem(d);
   DialogItemTableSetOptions(table, 3, 0, 0, 0);

   if (Conf.dialogs.headers)
     {
	di = DialogAddItem(table, DITEM_IMAGE);
	DialogItemSetColSpan(di, 2);
	DialogItemImageSetFile(di, "pix/bg.png");

	di = DialogAddItem(table, DITEM_TEXT);
	DialogItemSetText(di,
			  _("Enlightenment Desktop\n"
			    "Background Settings Dialog\n"));

	di = DialogAddItem(table, DITEM_SEPARATOR);
	DialogItemSetColSpan(di, 3);
     }

   di = DialogAddItem(table, DITEM_TEXT);
   DialogItemSetText(di, _("BG Colour\n"));

   di = DialogAddItem(table, DITEM_NONE);

   table2 = DialogAddItem(table, DITEM_TABLE);
   DialogItemTableSetOptions(table2, 4, 0, 0, 0);

   di = DialogAddItem(table2, DITEM_BUTTON);
   DialogItemSetText(di, _("Move to Front\n"));
   DialogItemSetCallback(di, CB_ConfigureFrontBG, 0, NULL);
   DialogBindKey(d, "F", CB_ConfigureFrontBG, 0);

   di = DialogAddItem(table2, DITEM_BUTTON);
   DialogItemSetText(di, _("Duplicate\n"));
   DialogItemSetCallback(di, CB_ConfigureNewBG, 0, NULL);

   di = DialogAddItem(table2, DITEM_BUTTON);
   DialogItemSetText(di, _("Unlist\n"));
   DialogItemSetCallback(di, CB_ConfigureDelBG, 0, NULL);

   di = DialogAddItem(table2, DITEM_BUTTON);
   DialogItemSetText(di, _("Delete File\n"));
   DialogItemSetCallback(di, CB_ConfigureDelBG, 0, NULL);
   DialogBindKey(d, "D", CB_ConfigureDelBG, 0);
   DialogBindKey(d, "Delete", CB_ConfigureDelBG, 1);

   di = DialogAddItem(table, DITEM_TEXT);
   DialogItemSetFill(di, 0, 0);
   DialogItemSetAlign(di, 1024, 512);
   DialogItemSetText(di, _("Red:\n"));

   di = w1 = tmp_w[0] = DialogAddItem(table, DITEM_SLIDER);
   DialogItemSliderSetBounds(di, 0, 255);
   DialogItemSliderSetUnits(di, 1);
   DialogItemSliderSetJump(di, 16);
   DialogItemSliderSetValPtr(di, &tmp_bg_r);

   di = w2 = tmp_w[1] = DialogAddItem(table, DITEM_CHECKBUTTON);
   DialogItemSetText(di, _("Use background image"));
   DialogItemCheckButtonSetPtr(di, &tmp_bg_image);

   di = DialogAddItem(table, DITEM_TEXT);
   DialogItemSetFill(di, 0, 0);
   DialogItemSetAlign(di, 1024, 512);
   DialogItemSetText(di, _("Green:\n"));

   di = w3 = tmp_w[2] = DialogAddItem(table, DITEM_SLIDER);
   DialogItemSliderSetBounds(di, 0, 255);
   DialogItemSliderSetUnits(di, 1);
   DialogItemSliderSetJump(di, 16);
   DialogItemSliderSetValPtr(di, &tmp_bg_g);

   di = w4 = tmp_w[3] = DialogAddItem(table, DITEM_CHECKBUTTON);
   DialogItemSetText(di, _("Keep aspect on scale"));
   DialogItemCheckButtonSetPtr(di, &tmp_bg_keep_aspect);

   di = DialogAddItem(table, DITEM_TEXT);
   DialogItemSetFill(di, 0, 0);
   DialogItemSetAlign(di, 1024, 512);
   DialogItemSetText(di, _("Blue:\n"));

   di = w5 = tmp_w[4] = DialogAddItem(table, DITEM_SLIDER);
   DialogItemSliderSetBounds(di, 0, 255);
   DialogItemSliderSetUnits(di, 1);
   DialogItemSliderSetJump(di, 16);
   DialogItemSliderSetValPtr(di, &tmp_bg_b);

   di = w6 = tmp_w[5] = DialogAddItem(table, DITEM_CHECKBUTTON);
   DialogItemSetText(di, _("Tile image across background"));
   DialogItemCheckButtonSetPtr(di, &tmp_bg_tile);

   di = DialogAddItem(table, DITEM_SEPARATOR);
   DialogItemSetColSpan(di, 3);

   di = DialogAddItem(table, DITEM_TEXT);
   DialogItemSetFill(di, 0, 0);
   DialogItemSetAlign(di, 512, 512);
   DialogItemSetColSpan(di, 2);
   DialogItemSetText(di,
		     _("Background\n" "Image\n" "Scaling\n" "and\n"
		       "Alignment\n"));

   table2 = DialogAddItem(table, DITEM_TABLE);
   DialogItemTableSetOptions(table2, 3, 0, 0, 0);

   di = DialogAddItem(table2, DITEM_NONE);

   di = slider = tmp_w[6] = DialogAddItem(table2, DITEM_SLIDER);
   DialogItemSliderSetMinLength(di, 10);
   DialogItemSliderSetBounds(di, 0, 1024);
   DialogItemSliderSetUnits(di, 1);
   DialogItemSliderSetJump(di, 64);
   DialogItemSliderSetValPtr(di, &tmp_bg_xjust);

   di = DialogAddItem(table2, DITEM_NONE);

   di = slider2 = tmp_w[7] = DialogAddItem(table2, DITEM_SLIDER);
   DialogItemSliderSetMinLength(di, 10);
   DialogItemSliderSetOrientation(di, 0);
   DialogItemSetFill(di, 0, 1);
   DialogItemSliderSetBounds(di, 0, 1024);
   DialogItemSliderSetUnits(di, 1);
   DialogItemSliderSetJump(di, 64);
   DialogItemSliderSetValPtr(di, &tmp_bg_yjust);

   di = bg_mini_disp = area = DialogAddItem(table2, DITEM_AREA);
   DialogItemAreaSetSize(di, 64, 48);

   DialogItemSetCallback(w1, CB_DesktopMiniDisplayRedraw, 0, (void *)area);
   DialogItemSetCallback(w2, CB_DesktopMiniDisplayRedraw, 0, (void *)area);
   DialogItemSetCallback(w3, CB_DesktopMiniDisplayRedraw, 0, (void *)area);
   DialogItemSetCallback(w4, CB_DesktopMiniDisplayRedraw, 0, (void *)area);
   DialogItemSetCallback(w5, CB_DesktopMiniDisplayRedraw, 0, (void *)area);
   DialogItemSetCallback(w6, CB_DesktopMiniDisplayRedraw, 0, (void *)area);
   DialogItemSetCallback(slider, CB_DesktopMiniDisplayRedraw, 0, (void *)area);
   DialogItemSetCallback(slider2, CB_DesktopMiniDisplayRedraw, 0, (void *)area);

   di = slider = tmp_w[8] = DialogAddItem(table2, DITEM_SLIDER);
   DialogItemSliderSetMinLength(di, 10);
   DialogItemSliderSetOrientation(di, 0);
   DialogItemSetFill(di, 0, 1);
   DialogItemSliderSetBounds(di, 0, 1024);
   DialogItemSliderSetUnits(di, 1);
   DialogItemSliderSetJump(di, 64);
   DialogItemSliderSetValPtr(di, &tmp_bg_yperc);
   DialogItemSetCallback(slider, CB_DesktopMiniDisplayRedraw, 0, (void *)area);

   di = DialogAddItem(table2, DITEM_NONE);

   di = slider = tmp_w[9] = DialogAddItem(table2, DITEM_SLIDER);
   DialogItemSliderSetMinLength(di, 10);
   DialogItemSliderSetBounds(di, 0, 1024);
   DialogItemSliderSetUnits(di, 1);
   DialogItemSliderSetJump(di, 64);
   DialogItemSliderSetValPtr(di, &tmp_bg_xperc);
   DialogItemSetCallback(slider, CB_DesktopMiniDisplayRedraw, 0, (void *)area);

   di = DialogAddItem(table, DITEM_SEPARATOR);
   DialogItemSetColSpan(di, 3);

   di = DialogAddItem(table, DITEM_CHECKBUTTON);
   DialogItemSetColSpan(di, 2);
   DialogItemSetText(di, _("Use dithering in Hi-Colour"));
   DialogItemCheckButtonSetPtr(di, &tmp_hiq);

   di = DialogAddItem(table, DITEM_CHECKBUTTON);
   DialogItemSetText(di, _("Background overrides theme"));
   DialogItemCheckButtonSetPtr(di, &tmp_userbg);

   di = label = DialogAddItem(table, DITEM_TEXT);
   DialogItemSetColSpan(di, 3);
   DialogItemSetAlign(di, 512, 512);
   Esnprintf(s, sizeof(s), _("Unused backgrounds freed after %2i:%02i:%02i"),
	     tmp_bg_timeout / 3600,
	     (tmp_bg_timeout / 60) - (60 * (tmp_bg_timeout / 3600)),
	     (tmp_bg_timeout) - (60 * (tmp_bg_timeout / 60)));
   DialogItemSetText(di, s);

   di = DialogAddItem(table, DITEM_SLIDER);
   DialogItemSetColSpan(di, 3);
   DialogItemSliderSetMinLength(di, 10);
   DialogItemSliderSetBounds(di, 0, 60 * 60 * 4);
   DialogItemSliderSetUnits(di, 30);
   DialogItemSliderSetJump(di, 60);
   DialogItemSliderSetValPtr(di, &tmp_bg_timeout);
   DialogItemSetCallback(di, CB_DesktopTimeout, 0, (void *)label);

   di = DialogAddItem(table, DITEM_SEPARATOR);
   DialogItemSetColSpan(di, 3);

   table2 = DialogAddItem(table, DITEM_TABLE);
   DialogItemTableSetOptions(table2, 2, 0, 0, 0);

   di = DialogAddItem(table2, DITEM_BUTTON);
   DialogItemSetFill(di, 0, 0);
   DialogItemSetText(di, "<-");
   DialogItemSetCallback(di, CB_BGPrev, 0, NULL);
   DialogBindKey(d, "Left", CB_BGPrev, 0);
   DialogBindKey(d, "BackSpace", CB_BGPrev, 0);

   di = DialogAddItem(table2, DITEM_BUTTON);
   DialogItemSetFill(di, 0, 0);
   DialogItemSetText(di, "->");
   DialogItemSetCallback(di, CB_BGNext, 0, NULL);
   DialogBindKey(d, "Right", CB_BGNext, 0);
   DialogBindKey(d, "space", CB_BGNext, 0);

   di = DialogAddItem(table, DITEM_BUTTON);
   DialogItemSetFill(di, 0, 0);
   DialogItemSetText(di, _("Pre-scan BG's"));
   DialogItemSetCallback(di, CB_BGScan, 0, NULL);

   table2 = DialogAddItem(table, DITEM_TABLE);
   DialogItemTableSetOptions(table2, 3, 0, 0, 0);

   di = DialogAddItem(table2, DITEM_BUTTON);
   DialogItemSetFill(di, 0, 0);
   DialogItemSetText(di, _("Sort by File"));
   DialogItemSetCallback(di, CB_BGSortFile, 0, NULL);

   di = DialogAddItem(table2, DITEM_BUTTON);
   DialogItemSetFill(di, 0, 0);
   DialogItemSetText(di, _("Sort by Attr."));
   DialogItemSetCallback(di, CB_BGSortAttrib, 0, NULL);

   di = DialogAddItem(table2, DITEM_BUTTON);
   DialogItemSetFill(di, 0, 0);
   DialogItemSetText(di, _("Sort by Image"));
   DialogItemSetCallback(di, CB_BGSortContent, 0, NULL);

   di = bg_sel = DialogAddItem(table, DITEM_AREA);
   DialogItemSetColSpan(di, 3);
   DialogItemAreaSetSize(di, 160, 56);
   DialogItemAreaSetEventFunc(di, CB_BGAreaEvent);

   num = 0;
   {
      Background        **bglist;

      bglist = (Background **) ListItemType(&num, LIST_TYPE_BACKGROUND);
      if (bglist)
	 Efree(bglist);
   }
   di = bg_sel_slider = DialogAddItem(table, DITEM_SLIDER);
   DialogItemSetColSpan(di, 3);
   DialogItemSliderSetBounds(di, 0, num * 4);
   DialogItemSliderSetUnits(di, 1);
   DialogItemSliderSetJump(di, 9);
   DialogItemSliderSetValPtr(di, &tmp_bg_sel_sliderval);
   DialogItemSetCallback(bg_sel_slider, CB_BGAreaSlide, 0, NULL);

   di = bg_filename = DialogAddItem(table, DITEM_TEXT);
   DialogItemSetColSpan(di, 3);
   DialogItemSetAlign(di, 512, 512);
   BG_DialogSetFileName(bg_filename);

   di = DialogAddItem(table, DITEM_SEPARATOR);
   DialogItemSetColSpan(di, 3);

   DialogAddButton(d, _("OK"), CB_ConfigureBG, 1, DIALOG_BUTTON_OK);
   DialogAddButton(d, _("Apply"), CB_ConfigureBG, 0, DIALOG_BUTTON_APPLY);
   DialogAddButton(d, _("Close"), CB_ConfigureBG, 1, DIALOG_BUTTON_CLOSE);
   DialogSetExitFunction(d, CB_ConfigureBG, -1);
   DialogBindKey(d, "Escape", DialogCallbackClose, 0);
   DialogBindKey(d, "Return", CB_ConfigureBG, 0);

   ShowDialog(d);

   CB_DesktopMiniDisplayRedraw(NULL, 1, area);
   BG_RedrawView();
   BGSettingsGoTo(tmp_bg);
}

/*
 * IPC functions
 */

static void
BackgroundSet1(const char *name, const char *params)
{
   const char         *p = params;
   char                type[FILEPATH_LEN_MAX];
   char                valu[FILEPATH_LEN_MAX];
   int                 len;
   Background         *bg;

   if (!p)
      return;

   bg = FindItem(name, 0, LIST_FINDBY_NAME, LIST_TYPE_BACKGROUND);
   if (!bg)
     {
	bg = BackgroundCreate(name, NULL, NULL, 0, 0, 0,
			      0, 0, 0, NULL, 0, 0, 0, 0, 0);
	if (!bg)
	  {
	     IpcPrintf("Error: could not create background '%s'.", name);
	     return;
	  }
     }

   type[0] = valu[0] = '\0';
   len = 0;
   sscanf(p, "%400s %4000s %n", type, valu, &len);
   p += len;

   if (!strcmp(type, "bg.solid"))
     {
	int                 r, b, g;

	r = g = b = 0;
	sscanf(p, "%i %i %i", &r, &g, &b);
	ESetColor(&(bg->bg_solid), r, g, b);
     }
   else if (!strcmp(type, "bg.file"))
     {
	if (bg->bg.file)
	   Efree(bg->bg.file);
	bg->bg.file = Estrdup(valu);
     }
   else if (!strcmp(type, "bg.tile"))
     {
	bg->bg_tile = atoi(valu);
     }
   else if (!strcmp(type, "bg.keep_aspect"))
     {
	bg->bg.keep_aspect = atoi(valu);
     }
   else if (!strcmp(type, "bg.xjust"))
     {
	bg->bg.xjust = atoi(valu);
     }
   else if (!strcmp(type, "bg.yjust"))
     {
	bg->bg.yjust = atoi(valu);
     }
   else if (!strcmp(type, "bg.xperc"))
     {
	bg->bg.xperc = atoi(valu);
     }
   else if (!strcmp(type, "bg.yperc"))
     {
	bg->bg.yperc = atoi(valu);
     }
   else if (!strcmp(type, "top.file"))
     {
	if (bg->top.file)
	   Efree(bg->top.file);
	bg->top.file = Estrdup(valu);
     }
   else if (!strcmp(type, "top.keep_aspect"))
     {
	bg->top.keep_aspect = atoi(valu);
     }
   else if (!strcmp(type, "top.xjust"))
     {
	bg->top.xjust = atoi(valu);
     }
   else if (!strcmp(type, "top.yjust"))
     {
	bg->top.yjust = atoi(valu);
     }
   else if (!strcmp(type, "top.xperc"))
     {
	bg->top.xperc = atoi(valu);
     }
   else if (!strcmp(type, "top.yperc"))
     {
	bg->top.yperc = atoi(valu);
     }
   else
     {
	IpcPrintf("Error: unknown background value type '%s'.", type);
     }
   autosave();
}

static void
BackgroundSet2(const char *name, const char *params)
{
   Background         *bg;
   XColor              xclr;
   unsigned int        i;
   int                 r, g, b;
   char                bgf[FILEPATH_LEN_MAX], topf[FILEPATH_LEN_MAX];
   int                 updated, tile, keep_aspect, tkeep_aspect;
   int                 xjust, yjust, xperc, yperc;
   int                 txjust, tyjust, txperc, typerc;

   if (params == NULL)
      return;

   bgf[0] = topf[0] = '\0';
   r = g = b = 99;
   i = sscanf(params,
	      "%i %i %i %4000s %i %i %i %i %i %i %4000s %i %i %i %i %i",
	      &r, &g, &b,
	      bgf, &tile, &keep_aspect, &xjust, &yjust, &xperc, &yperc,
	      topf, &tkeep_aspect, &txjust, &tyjust, &txperc, &typerc);
   ESetColor(&xclr, r, g, b);

   bg = FindItem(name, 0, LIST_FINDBY_NAME, LIST_TYPE_BACKGROUND);
   if (bg)
     {
	updated = BackgroundModify(bg, &xclr, bgf, tile, keep_aspect, xjust,
				   yjust, xperc, yperc, topf, tkeep_aspect,
				   txjust, tyjust, txperc, typerc);
	if (updated)
	  {
	     for (i = 0; i < DesksGetNumber(); i++)
	       {
		  Desk               *dsk = DeskGet(i);

		  if (DeskGetBackground(dsk) == bg)
		     DeskSetBg(dsk, bg, 0);
	       }
	  }
     }
   else
     {
	bg = BackgroundCreate(name, &xclr, bgf, tile, keep_aspect, xjust,
			      yjust, xperc, yperc, topf, tkeep_aspect,
			      txjust, tyjust, txperc, typerc);
     }
}

static void
BackgroundsIpc(const char *params, Client * c __UNUSED__)
{
   const char         *p;
   char                cmd[128], prm[128], buf[4096];
   int                 i, len, num;
   Background         *bg;

   cmd[0] = prm[0] = '\0';
   p = params;
   if (p)
     {
	len = 0;
	sscanf(p, "%100s %100s %n", cmd, prm, &len);
	p += len;
     }

   if (!p || cmd[0] == '?')
     {
	for (i = 0; i < (int)DesksGetNumber(); i++)
	  {
	     bg = DeskGetBackground(DeskGet(i));
	     if (bg)
		IpcPrintf("%i %s\n", i, BackgroundGetName(bg));
	  }
     }
   else if (!strncmp(cmd, "apply", 2))
     {
	Window              win;

	bg = FindItem(prm, 0, LIST_FINDBY_NAME, LIST_TYPE_BACKGROUND);
	if (bg)
	  {
	     win = None;
	     sscanf(p, "%lx", &win);
	     if (win)
		BackgroundApply2(bg, win);
	  }
     }
   else if (!strncmp(cmd, "cfg", 2))
     {
	SettingsBackground(DeskGetBackground(DesksGetCurrent()));
     }
   else if (!strncmp(cmd, "del", 2))
     {
	BackgroundDestroyByName(prm);
     }
   else if (!strncmp(cmd, "list", 2))
     {
	Background        **lst;

	lst = (Background **) ListItemType(&num, LIST_TYPE_BACKGROUND);
	for (i = 0; i < num; i++)
	  {
	     IpcPrintf("%s\n", BackgroundGetName(lst[i]));
	  }
	if (lst)
	   Efree(lst);
     }
   else if (!strncmp(cmd, "load", 2))
     {
	bg = FindItem(prm, 0, LIST_FINDBY_NAME, LIST_TYPE_BACKGROUND);
	if (bg)
	  {
	     IpcPrintf("Background already defined\n");
	  }
	else
	  {
	     bg = BrackgroundCreateFromImage(prm, p, NULL, 0);
	  }
     }
   else if (!strncmp(cmd, "set", 2))
     {
	BackgroundSet1(prm, p);
     }
   else if (!strncmp(cmd, "show", 2))
     {
	bg = FindItem(prm, 0, LIST_FINDBY_NAME, LIST_TYPE_BACKGROUND);

	if (bg)
	  {
	     BackgroundGetInfoString1(bg, buf, sizeof(buf));
	     IpcPrintf("%s\n", buf);
	  }
	else
	   IpcPrintf("Error: background '%s' does not exist.", prm);
     }
   else if (!strcmp(cmd, "use"))
     {
	if (!strcmp(prm, "-"))
	   bg = NULL;
	else
	   bg = FindItem(prm, 0, LIST_FINDBY_NAME, LIST_TYPE_BACKGROUND);

	num = DesksGetCurrentNum();
	sscanf(p, "%d %n", &num, &len);
	DeskSetBg(DeskGet(num), bg, 1);
	autosave();
     }
   else if (!strncmp(cmd, "xget", 2))
     {
	bg = FindItem(prm, 0, LIST_FINDBY_NAME, LIST_TYPE_BACKGROUND);

	if (bg)
	  {
	     BackgroundGetInfoString2(bg, buf, sizeof(buf));
	     IpcPrintf("%s\n", buf);
	  }
	else
	   IpcPrintf("Error: background '%s' does not exist.", prm);
     }
   else if (!strncmp(cmd, "xset", 2))
     {
	BackgroundSet2(prm, p);
     }
   else
     {
	/* Compatibility with pre- 0.16.8 clients */
	BackgroundSet1(cmd, atword(params, 2));
     }
}

static void
IPC_BackgroundUse(const char *params, Client * c __UNUSED__)
{
   char                param1[FILEPATH_LEN_MAX], w[FILEPATH_LEN_MAX];
   Background         *bg;
   int                 i, wd;

   word(params, 1, param1);

   bg = FindItem(param1, 0, LIST_FINDBY_NAME, LIST_TYPE_BACKGROUND);
   if (!bg)
      return;

   for (wd = 2;; wd++)
     {
	w[0] = 0;
	word(params, wd++, w);
	if (!w[0])
	   break;
	i = atoi(w);
	DeskSetBg(DeskGet(i), bg, 1);
     }
   autosave();
}

#if ENABLE_COLOR_MODIFIERS
static void
IPC_BackgroundColormodifierSet(const char *params, Client * c __UNUSED__)
{
   Background         *bg;
   ColorModifierClass *cm;
   int                 i;
   char                buf[FILEPATH_LEN_MAX], buf2[FILEPATH_LEN_MAX];

   if (params == NULL)
      return;

   sscanf(params, "%1000s %1000s", buf, buf2);
   bg = FindItem(buf, 0, LIST_FINDBY_NAME, LIST_TYPE_BACKGROUND);
   cm = FindItem(buf2, 0, LIST_FINDBY_NAME, LIST_TYPE_COLORMODIFIER);
   if ((bg) && (bg->cmclass != cm))
     {
	if (!strcmp(buf, "(null)"))
	  {
	     bg->cmclass->ref_count--;
	     bg->cmclass = NULL;
	  }
	else if (cm)
	  {
	     bg->cmclass->ref_count--;
	     bg->cmclass = cm;
	  }
	if (bg->pmap)
	   imlib_free_pixmap_and_mask(bg->pmap);
	bg->pmap = 0;
	for (i = 0; i < DesksGetNumber(); i++)
	  {
	     if ((desks.desk[i].bg == bg) && (desks.desk[i].viewable))
		DeskRefresh(DeskGet(i));
	  }
     }
}

static void
IPC_BackgroundColormodifierGet(const char *params, Client * c)
{
   char                param1[FILEPATH_LEN_MAX], buf[FILEPATH_LEN_MAX];
   Background         *bg;

   if (params == NULL)
      return;

   sscanf(params, "%1000s", param1);
   bg = FindItem(param1, 0, LIST_FINDBY_NAME, LIST_TYPE_BACKGROUND);
   Esnprintf(buf, sizeof(buf), "(null)");
   if ((bg) && (bg->cmclass))
      Esnprintf(buf, sizeof(buf), "%s", bg->cmclass->name);
   CommsSend(c, buf);
}
#endif

IpcItem             BackgroundsIpcArray[] = {
   {
    BackgroundsIpc,
    "background", "bg",
    "Background commands",
    "  background                       Show current background\n"
    "  background apply <name> <win>    Apply background to window\n"
    "  background cfg                   Configure backgrounds\n"
    "  background del <name>            Delete background\n"
    "  background list                  Show all background\n"
    "  background load <name> <file>    Load new wallpaper from file\n"
    "  background set <name> ...        Set background parameters\n"
    "  background show <name>           Show background info\n"
    "  background use <name> <desks...> Switch to background <name>\n"
    "  background xget <name>           Special show background parameters\n"
    "  background xset <name> ...       Special set background parameters\n"}
   ,
   {
    IPC_BackgroundUse, "use_bg", NULL, "Deprecated - do not use", NULL}
   ,
#if ENABLE_COLOR_MODIFIERS
   {IPC_BackgroundColormodifierSet, "set_bg_colmod", NULL, "TBD", NULL}
   ,
   {IPC_BackgroundColormodifierGet, "get_bg_colmod", NULL, "TBD", NULL}
   ,
#endif
};
#define N_IPC_FUNCS (sizeof(BackgroundsIpcArray)/sizeof(IpcItem))

/*
 * Configuration items
 */
static const CfgItem BackgroundsCfgItems[] = {
   CFG_ITEM_BOOL(Conf.backgrounds, hiquality, 1),
   CFG_ITEM_BOOL(Conf.backgrounds, user, 1),
   CFG_ITEM_INT(Conf.backgrounds, timeout, 240),
};
#define N_CFG_ITEMS (sizeof(BackgroundsCfgItems)/sizeof(CfgItem))

/*
 * Module descriptor
 */
EModule             ModBackgrounds = {
   "backgrounds", "bg",
   BackgroundsSighan,
   {N_IPC_FUNCS, BackgroundsIpcArray},
   {N_CFG_ITEMS, BackgroundsCfgItems}
};
