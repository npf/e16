/*
 * Copyright (C) 2000-2007 Carsten Haitzler, Geoff Harrison and various contributors
 * Copyright (C) 2004-2012 Kim Woelders
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
#include "E.h"
#include "animation.h"
#include "desktops.h"
#include "dialog.h"
#include "ecompmgr.h"
#include "eimage.h"
#include "emodule.h"
#include "settings.h"
#include "xwin.h"
#include <math.h>

#ifndef M_PI_2
#define M_PI_2 (3.141592654 / 2)
#endif

#define FX_OP_ENABLE  1		/* Enable, start */
#define FX_OP_DISABLE 2		/* Disable, stop */
#define FX_OP_START   3		/* Start (if enabled) */
#define FX_OP_PAUSE   4
#define FX_OP_DESK    5

typedef struct {
   const char         *name;
   void                (*init_func) (const char *name);
   void                (*desk_func) (void);
   void                (*quit_func) (void);
   char                enabled;
   char                active;
} FXHandler;

#if USE_COMPOSITE
/* As of composite 0.4 we need to set the clip region */
#define SET_GC_CLIP(eo, gc) ECompMgrWinClipToGC(eo, gc)
#else
#define SET_GC_CLIP(eo, gc)
#endif

/****************************** RIPPLES *************************************/

#define fx_ripple_waterh 64
static Pixmap       fx_ripple_above = None;
static Win          fx_ripple_win = NULL;
static int          fx_ripple_count = 0;

static int
FX_ripple_timeout(EObj * eo __UNUSED__, int remaining __UNUSED__,
		  void *state __UNUSED__)
{
   static double       incv = 0, inch = 0;
   static GC           gc1 = 0, gc = 0;
   int                 y;
   EObj               *bgeo;

   bgeo = DeskGetBackgroundObj(DesksGetCurrent());

   if (fx_ripple_above == None)
     {
	XGCValues           gcv;

	fx_ripple_win = EobjGetWin(bgeo);

	fx_ripple_above =
	   ECreatePixmap(fx_ripple_win, WinGetW(VROOT),
			 fx_ripple_waterh * 2, 0);
	EXFreeGC(gc);
	EXFreeGC(gc1);
	gcv.subwindow_mode = IncludeInferiors;
	gc = EXCreateGC(WinGetXwin(fx_ripple_win), GCSubwindowMode, &gcv);
	gc1 = EXCreateGC(WinGetXwin(fx_ripple_win), 0L, &gcv);
     }

   if (fx_ripple_count == 0)
      XCopyArea(disp, WinGetXwin(fx_ripple_win), fx_ripple_above, gc, 0,
		WinGetH(VROOT) - (fx_ripple_waterh * 3), WinGetW(VROOT),
		fx_ripple_waterh * 2, 0, 0);

   fx_ripple_count++;
   if (fx_ripple_count > 32)
      fx_ripple_count = 0;

   incv += 0.40;
   if (incv > (M_PI_2 * 4))
      incv = 0;
   inch += 0.32;
   if (inch > (M_PI_2 * 4))
      inch = 0;

   SET_GC_CLIP(bgeo, gc1);

   for (y = 0; y < fx_ripple_waterh; y++)
     {
	double              aa, a, p;
	int                 yoff, off, yy;

	p = (((double)(fx_ripple_waterh - y)) / ((double)fx_ripple_waterh));
	a = p * p * 48 + incv;
	yoff = y + (int)(sin(a) * 7) + 1;
	yy = (fx_ripple_waterh * 2) - yoff;
	aa = p * p * 64 + inch;
	off = (int)(sin(aa) * 10 * (1 - p));
	XCopyArea(disp, fx_ripple_above, WinGetXwin(fx_ripple_win), gc1, 0, yy,
		  WinGetW(VROOT), 1, off,
		  WinGetH(VROOT) - fx_ripple_waterh + y);
     }

   return 4;
}

static void
FX_Ripple_Init(const char *name __UNUSED__)
{
   fx_ripple_count = 0;
   AnimatorAdd(NULL, ANIM_FX_RIPPLES, FX_ripple_timeout, -1, 0, 0, NULL);
}

static void
FX_Ripple_Desk(void)
{
   EFreePixmap(fx_ripple_above);
   fx_ripple_count = 0;
   fx_ripple_above = None;
}

static void
FX_Ripple_Quit(void)
{
   AnimatorsDelCatAll(ANIM_FX_RIPPLES, 0);
   if (!fx_ripple_win)
      return;
   EClearArea(fx_ripple_win, 0, WinGetH(VROOT) - fx_ripple_waterh,
	      WinGetW(VROOT), fx_ripple_waterh);
   FX_Ripple_Desk();
}

/****************************** WAVES ***************************************/
/* by tsade :)                                                              */
/****************************************************************************/

#define FX_WAVE_WATERH 64
#define FX_WAVE_WATERW 64
#define FX_WAVE_DEPTH  10
#define FX_WAVE_GRABH  (FX_WAVE_WATERH + FX_WAVE_DEPTH)
#define FX_WAVE_CROSSPERIOD 0.42
static Pixmap       fx_wave_above = None;
static Win          fx_wave_win = NULL;
static int          fx_wave_count = 0;

static int
FX_Wave_timeout(EObj * eo __UNUSED__, int remaining __UNUSED__,
		void *state __UNUSED__)
{
   /* Variables */
   static double       incv = 0, inch = 0;
   static double       incx = 0;
   double              incx2;
   static GC           gc1 = 0, gc = 0;
   int                 y;
   EObj               *bgeo;

   bgeo = DeskGetBackgroundObj(DesksGetCurrent());

   /* Check to see if we need to create stuff */
   if (!fx_wave_above)
     {
	XGCValues           gcv;

	fx_wave_win = EobjGetWin(bgeo);

	fx_wave_above =
	   ECreatePixmap(fx_wave_win, WinGetW(VROOT), FX_WAVE_WATERH * 2, 0);

	EXFreeGC(gc);
	EXFreeGC(gc1);
	gcv.subwindow_mode = IncludeInferiors;
	gc = EXCreateGC(WinGetXwin(fx_wave_win), GCSubwindowMode, &gcv);
	gc1 = EXCreateGC(WinGetXwin(fx_wave_win), 0L, &gcv);
     }

   /* On the zero, grab the desktop again. */
   if (fx_wave_count == 0)
     {
	XCopyArea(disp, WinGetXwin(fx_wave_win), fx_wave_above, gc, 0,
		  WinGetH(VROOT) - (FX_WAVE_WATERH * 3), WinGetW(VROOT),
		  FX_WAVE_WATERH * 2, 0, 0);
     }

   /* Increment and roll the counter */
   fx_wave_count++;
   if (fx_wave_count > 32)
      fx_wave_count = 0;

   /* Increment and roll some other variables */
   incv += 0.40;
   if (incv > (M_PI_2 * 4))
      incv = 0;

   inch += 0.32;
   if (inch > (M_PI_2 * 4))
      inch = 0;

   incx += 0.32;
   if (incx > (M_PI_2 * 4))
      incx = 0;

   SET_GC_CLIP(bgeo, gc1);

   /* Copy the area to correct bugs */
   if (fx_wave_count == 0)
     {
	XCopyArea(disp, fx_wave_above, WinGetXwin(fx_wave_win), gc1, 0,
		  WinGetH(VROOT) - FX_WAVE_GRABH, WinGetW(VROOT),
		  FX_WAVE_DEPTH * 2, 0, WinGetH(VROOT) - FX_WAVE_GRABH);
     }

   /* Go through the bottom couple (FX_WAVE_WATERH) lines of the window */
   for (y = 0; y < FX_WAVE_WATERH; y++)
     {
	/* Variables */
	double              aa, a, p;
	int                 yoff, off, yy;
	int                 x;

	/* Figure out the side-to-side movement */
	p = (((double)(FX_WAVE_WATERH - y)) / ((double)FX_WAVE_WATERH));
	a = p * p * 48 + incv;
	yoff = y + (int)(sin(a) * 7) + 1;
	yy = (FX_WAVE_WATERH * 2) - yoff;
	aa = p * p * FX_WAVE_WATERH + inch;
	off = (int)(sin(aa) * 10 * (1 - p));

	/* Set up the next part */
	incx2 = incx;

	/* Go through the width of the screen, in block sizes */
	for (x = 0; x < WinGetW(VROOT); x += FX_WAVE_WATERW)
	  {
	     /* Variables */
	     int                 sx;

	     /* Add something to incx2 and roll it */
	     incx2 += FX_WAVE_CROSSPERIOD;

	     if (incx2 > (M_PI_2 * 4))
		incx2 = 0;

	     /* Figure it out */
	     sx = (int)(sin(incx2) * FX_WAVE_DEPTH);

	     /* Display this block */
	     XCopyArea(disp, fx_wave_above, WinGetXwin(fx_wave_win), gc1, x, yy,	/* x, y */
		       FX_WAVE_WATERW, 1,	/* w, h */
		       off + x, WinGetH(VROOT) - FX_WAVE_WATERH + y + sx	/* dx, dy */
		);
	  }
     }

   return 4;
}

static void
FX_Waves_Init(const char *name __UNUSED__)
{
   fx_wave_count = 0;
   AnimatorAdd(NULL, ANIM_FX_WAVES, FX_Wave_timeout, -1, 0, 0, NULL);
}

static void
FX_Waves_Desk(void)
{
   EFreePixmap(fx_wave_above);
   fx_wave_count = 0;
   fx_wave_above = None;
}

static void
FX_Waves_Quit(void)
{
   AnimatorsDelCatAll(ANIM_FX_WAVES, 0);
   if (!fx_wave_win)
      return;
   EClearArea(fx_wave_win, 0, WinGetH(VROOT) - FX_WAVE_WATERH,
	      WinGetW(VROOT), FX_WAVE_WATERH);
   FX_Waves_Desk();
}

/****************************************************************************/

#define fx_rip fx_handlers[0]
#define fx_wav fx_handlers[1]

static FXHandler    fx_handlers[] = {
   {"ripples",
    FX_Ripple_Init, FX_Ripple_Desk, FX_Ripple_Quit,
    0, 0},
   {"waves",
    FX_Waves_Init, FX_Waves_Desk, FX_Waves_Quit,
    0, 0},
};
#define N_FX_HANDLERS (sizeof(fx_handlers)/sizeof(FXHandler))

/****************************** Effect handlers *****************************/

static void
FX_Op(FXHandler * fxh, int op)
{
   switch (op)
     {
     case FX_OP_ENABLE:
	if (fxh->enabled)
	   break;
	fxh->enabled = 1;
	goto do_start;

     case FX_OP_DISABLE:
	if (!fxh->enabled)
	   break;
	fxh->enabled = 0;
	goto do_stop;

     case FX_OP_START:
	if (!fxh->enabled)
	   break;
      do_start:
	if (fxh->active)
	   break;
	fxh->init_func(fxh->name);
	fxh->active = 1;
	break;

     case FX_OP_PAUSE:
	if (!fxh->enabled)
	   break;
      do_stop:
	if (!fxh->active)
	   break;
	fxh->quit_func();
	fxh->active = 0;
	break;

     case FX_OP_DESK:
	if (!fxh->enabled)
	   break;
	fxh->desk_func();
	break;
     }
}

static void
FX_OpForEach(int op)
{
   unsigned int        i;

   for (i = 0; i < N_FX_HANDLERS; i++)
      FX_Op(&fx_handlers[i], op);
}

static void
FxCfgFunc(void *item __UNUSED__, const char *value)
{
   FXHandler          *fxh = NULL;

   if (item == &fx_rip.enabled)
      fxh = &fx_rip;
   else if (item == &fx_wav.enabled)
      fxh = &fx_wav;
   if (!fxh)
      return;

   FX_Op(fxh, atoi(value) ? FX_OP_ENABLE : FX_OP_DISABLE);
}

/****************************************************************************/

/*
 * Fx Module
 */

static void
FxSighan(int sig, void *prm __UNUSED__)
{
   switch (sig)
     {
     case ESIGNAL_START:
	FX_OpForEach(FX_OP_START);
	break;
     case ESIGNAL_DESK_SWITCH_START:
	break;
     case ESIGNAL_DESK_SWITCH_DONE:
	FX_OpForEach(FX_OP_DESK);
	break;
     case ESIGNAL_ANIMATION_SUSPEND:
	FX_OpForEach(FX_OP_PAUSE);
	break;
     case ESIGNAL_ANIMATION_RESUME:
	FX_OpForEach(FX_OP_START);
	break;
     }
}

#if ENABLE_DIALOGS
static char         tmp_effect_ripples;
static char         tmp_effect_waves;

static void
CB_ConfigureFX(Dialog * d __UNUSED__, int val, void *data __UNUSED__)
{
   if (val >= 2)
      return;

   FX_Op(&fx_rip, tmp_effect_ripples ? FX_OP_ENABLE : FX_OP_DISABLE);
   FX_Op(&fx_wav, tmp_effect_waves ? FX_OP_ENABLE : FX_OP_DISABLE);

   autosave();
}

static void
_DlgFillFx(Dialog * d __UNUSED__, DItem * table, void *data __UNUSED__)
{
   DItem              *di;

   tmp_effect_ripples = fx_rip.enabled;
   tmp_effect_waves = fx_wav.enabled;

   DialogItemTableSetOptions(table, 1, 0, 0, 0);

   /* Effects */
   di = DialogAddItem(table, DITEM_TEXT);
   DialogItemSetText(di, _("Effects"));
   di = DialogAddItem(table, DITEM_CHECKBUTTON);
   DialogItemSetText(di, _("Ripples"));
   DialogItemCheckButtonSetPtr(di, &tmp_effect_ripples);
   di = DialogAddItem(table, DITEM_CHECKBUTTON);
   DialogItemSetText(di, _("Waves"));
   DialogItemCheckButtonSetPtr(di, &tmp_effect_waves);
}

const DialogDef     DlgFx = {
   "CONFIGURE_FX",
   N_("FX"),
   N_("Special FX Settings"),
   SOUND_SETTINGS_FX,
   "pix/fx.png",
   N_("Enlightenment Special Effects\n" "Settings Dialog"),
   _DlgFillFx,
   DLG_OAC, CB_ConfigureFX,
};
#endif /* ENABLE_DIALOGS */

#define CFR_FUNC_BOOL(conf, name, dflt, func) \
    { #name, &conf, ITEM_TYPE_BOOL, dflt, func }

static const CfgItem FxCfgItems[] = {
   CFR_FUNC_BOOL(fx_handlers[0].enabled, ripples.enabled, 0, FxCfgFunc),
   CFR_FUNC_BOOL(fx_handlers[1].enabled, waves.enabled, 0, FxCfgFunc),
};
#define N_CFG_ITEMS (sizeof(FxCfgItems)/sizeof(CfgItem))

/*
 * Module descriptor
 */
extern const EModule ModEffects;

const EModule       ModEffects = {
   "effects", "fx",
   FxSighan,
   {0, NULL},
   {N_CFG_ITEMS, FxCfgItems}
};
