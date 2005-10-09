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
#include "E.h"
#include "buttons.h"
#include "desktops.h"
#include "emodule.h"
#include "eobj.h"
#include "ewins.h"
#include "tooltips.h"
#include "xwin.h"
#include <time.h>
#if USE_XRANDR
#include <X11/extensions/Xrandr.h>
#endif

#define EDESK_EVENT_MASK \
  (ButtonPressMask | ButtonReleaseMask | \
   EnterWindowMask | LeaveWindowMask | PointerMotionMask | ButtonMotionMask | \
   SubstructureNotifyMask | SubstructureRedirectMask | PropertyChangeMask)

#define ENLIGHTENMENT_CONF_NUM_DESKTOPS 32

typedef struct _desktops
{
   Desk               *current;
   Desk               *previous;
   Desk               *desk[ENLIGHTENMENT_CONF_NUM_DESKTOPS];
   unsigned int        order[ENLIGHTENMENT_CONF_NUM_DESKTOPS];
   Background         *bg[ENLIGHTENMENT_CONF_NUM_DESKTOPS];
   int                 drag_x0, drag_y0;
}
Desktops;

static void         DeskRaise(unsigned int num);
static void         DeskLower(unsigned int num);
static void         DeskHandleEvents(XEvent * ev, void *prm);
static void         DeskButtonCallback(EObj * eo, XEvent * ev,
				       ActionClass * ac);

/* The desktops */
static Desktops     desks;

#define _DeskGet(d) (desks.desk[d])

static void
DeskControlsCreate(Desk * dsk)
{
   char                s[512];
   ActionClass        *ac, *ac2, *ac3;
   ImageClass         *ic, *ic2, *ic3, *ic4;
   Button             *b;
   Action             *a;
   int                 x[3], y[3], w[3], h[3], m, n, o;
   const char         *t;

   if (Conf.desks.dragdir < 0 || Conf.desks.dragdir > 3)
      Conf.desks.dragdir = 2;
   if (Conf.desks.dragbar_ordering < 0 || Conf.desks.dragbar_ordering > 5)
      Conf.desks.dragbar_ordering = 1;
   if (Conf.desks.dragbar_width < 0)
      Conf.desks.dragbar_width = 0;
   else if (Conf.desks.dragbar_width > 64)
      Conf.desks.dragbar_width = 64;
   if (Conf.desks.dragbar_length < 0)
      Conf.desks.dragbar_length = 0;
   else if (Conf.desks.dragbar_length > VRoot.w)
      Conf.desks.dragbar_length = VRoot.w;

   Esnprintf(s, sizeof(s), "DRAGBAR_DESKTOP_%i", dsk->num);
   ac = FindItem(s, 0, LIST_FINDBY_NAME, LIST_TYPE_ACLASS);
   if (!ac)
     {
	ac = ActionclassCreate(s, 0);
	a = ActionCreate(EVENT_MOUSE_DOWN, 0, 0, 0, 1, 0, NULL, NULL);
	ActionclassAddAction(ac, a);

	Esnprintf(s, sizeof(s), "desk drag %i", dsk->num);
	ActionAddTo(a, s);

	a = ActionCreate(EVENT_MOUSE_DOWN, 0, 0, 0, 3, 0, NULL, NULL);
	ActionclassAddAction(ac, a);
	ActionAddTo(a, "menus show deskmenu");

	a = ActionCreate(EVENT_MOUSE_DOWN, 0, 0, 0, 2, 0, NULL, NULL);
	ActionclassAddAction(ac, a);
	ActionAddTo(a, "menus show windowlist");

	if (dsk->num > 0)
	  {
	     t = _("Hold down the mouse button and drag\n"
		   "the mouse to be able to drag the desktop\n"
		   "back and forth.\n"
		   "Click right mouse button for a list of all\n"
		   "Desktops and their applications.\n"
		   "Click middle mouse button for a list of all\n"
		   "applications currently running.\n");
	     ActionclassSetTooltipString(ac, t);
	  }
	else
	  {
	     t = _("This is the Root desktop.\n"
		   "You cannot drag the root desktop around.\n"
		   "Click right mouse button for a list of all\n"
		   "Desktops and their applications.\n"
		   "Click middle mouse button for a list of all\n"
		   "applications currently running.\n");
	     ActionclassSetTooltipString(ac, t);
	  }
     }

   Esnprintf(s, sizeof(s), "RAISEBUTTON_DESKTOP_%i", dsk->num);
   ac2 = FindItem(s, 0, LIST_FINDBY_NAME, LIST_TYPE_ACLASS);
   if (!ac2)
     {
	ac2 = ActionclassCreate(s, 0);
	a = ActionCreate(EVENT_MOUSE_UP, 1, 0, 1, 0, 0, NULL, NULL);
	ActionclassAddAction(ac2, a);

	Esnprintf(s, sizeof(s), "desk raise %i", dsk->num);
	ActionAddTo(a, s);
	t = _("Click here to raise this desktop\nto the top.\n");
	ActionclassSetTooltipString(ac2, t);
     }

   Esnprintf(s, sizeof(s), "LOWERBUTTON_DESKTOP_%i", dsk->num);
   ac3 = FindItem(s, 0, LIST_FINDBY_NAME, LIST_TYPE_ACLASS);
   if (!ac3)
     {
	ac3 = ActionclassCreate(s, 0);
	a = ActionCreate(EVENT_MOUSE_UP, 1, 0, 1, 0, 0, NULL, NULL);
	ActionclassAddAction(ac3, a);

	Esnprintf(s, sizeof(s), "desk lower %i", dsk->num);
	ActionAddTo(a, s);
	t = _("Click here to lower this desktop\nto the bottom.\n");
	ActionclassSetTooltipString(ac3, t);
     }

   if (Conf.desks.dragdir < 2)
     {
	ic = ImageclassFind("DESKTOP_DRAGBUTTON_VERT", 0);
	ic2 = ImageclassFind("DESKTOP_RAISEBUTTON_VERT", 0);
	ic3 = ImageclassFind("DESKTOP_LOWERBUTTON_VERT", 0);
	ic4 = ImageclassFind("DESKTOP_DESKRAY_VERT", 0);
     }
   else
     {
	ic = ImageclassFind("DESKTOP_DRAGBUTTON_HORIZ", 0);
	ic2 = ImageclassFind("DESKTOP_RAISEBUTTON_HORIZ", 0);
	ic3 = ImageclassFind("DESKTOP_LOWERBUTTON_HORIZ", 0);
	ic4 = ImageclassFind("DESKTOP_DESKRAY_HORIZ", 0);
     }

   switch (Conf.desks.dragbar_ordering)
     {
     case 0:
	m = 0;
	n = 1;
	o = 2;
	break;
     case 1:
	m = 0;
	n = 2;
	o = 1;
	break;
     case 2:
	m = 2;
	n = 0;
	o = 1;
	break;
     case 3:
	m = 1;
	n = 0;
	o = 2;
	break;
     case 4:
	m = 1;
	n = 2;
	o = 0;
	break;
     case 5:
	m = 2;
	n = 1;
	o = 0;
	break;
     default:
	m = 0;
	n = 1;
	o = 2;
	break;
     }

   switch (Conf.desks.dragdir)
     {
     case 0:
	w[0] = w[1] = w[2] = h[0] = h[1] = Conf.desks.dragbar_width;
	if (Conf.desks.dragbar_length == 0)
	   h[2] = VRoot.h - (Conf.desks.dragbar_width * 2);
	else
	   h[2] = Conf.desks.dragbar_length;
	x[0] = x[1] = x[2] = 0;
	y[m] = 0;
	y[n] = y[m] + h[m];
	y[o] = y[n] + h[n];
	break;
     case 1:
	w[0] = w[1] = w[2] = h[0] = h[1] = Conf.desks.dragbar_width;
	if (Conf.desks.dragbar_length == 0)
	   h[2] = VRoot.h - (Conf.desks.dragbar_width * 2);
	else
	   h[2] = Conf.desks.dragbar_length;
	x[0] = x[1] = x[2] = VRoot.w - Conf.desks.dragbar_width;
	y[m] = 0;
	y[n] = y[m] + h[m];
	y[o] = y[n] + h[n];
	break;
     case 2:
	h[0] = h[1] = h[2] = w[0] = w[1] = Conf.desks.dragbar_width;
	if (Conf.desks.dragbar_length == 0)
	   w[2] = VRoot.w - (Conf.desks.dragbar_width * 2);
	else
	   w[2] = Conf.desks.dragbar_length;
	y[0] = y[1] = y[2] = 0;
	x[m] = 0;
	x[n] = x[m] + w[m];
	x[o] = x[n] + w[n];
	break;
     case 3:
	h[0] = h[1] = h[2] = w[0] = w[1] = Conf.desks.dragbar_width;
	if (Conf.desks.dragbar_length == 0)
	   w[2] = VRoot.w - (Conf.desks.dragbar_width * 2);
	else
	   w[2] = Conf.desks.dragbar_length;
	y[0] = y[1] = y[2] = VRoot.h - Conf.desks.dragbar_width;
	x[m] = 0;
	x[n] = x[m] + w[m];
	x[o] = x[n] + w[n];
	break;
     default:
	break;
     }

   b = NULL;

   if (Conf.desks.dragbar_width > 0)
     {
	b = ButtonCreate("_DESKTOP_DRAG_CONTROL", 1, ic2, ac2, NULL, NULL,
			 -1, FLAG_FIXED, 1, 99999, 1, 99999, 0, 0, x[0], 0,
			 y[0], 0, 0, w[0], 0, h[0], 0, dsk->num, 0);
	b = ButtonCreate("_DESKTOP_DRAG_CONTROL", 1, ic3, ac3, NULL, NULL,
			 -1, FLAG_FIXED, 1, 99999, 1, 99999, 0, 0, x[1], 0,
			 y[1], 0, 0, w[1], 0, h[1], 0, dsk->num, 0);
	b = ButtonCreate("_DESKTOP_DRAG_CONTROL", 1, ic, ac, NULL, NULL,
			 -1, FLAG_FIXED, 1, 99999, 1, 99999, 0, 0, x[2], 0,
			 y[2], 0, 0, w[2], 0, h[2], 0, dsk->num, 0);
	ButtonSetCallback(b, DeskButtonCallback, EoObj(dsk));
     }

#if 0				/* What is this anyway? */
   if (dsk->num > 0)
     {
	if (Conf.desks.dragdir == 0)
	  {
	     b = ButtonCreate("_DESKTOP_DESKRAY_DRAG_CONTROL", 2, ic4, ac,
			      NULL, NULL, 1, FLAG_FIXED_VERT, 1, 99999, 1,
			      99999, 0, 0, EoGetX(dsk), 0, EoGetY(dsk),
			      0, 0, 0, 0, 0, 1, 0, 1);
	  }
	else if (Conf.desks.dragdir == 1)
	  {
	     b = ButtonCreate("_DESKTOP_DESKRAY_DRAG_CONTROL", 2, ic4, ac,
			      NULL, NULL, 1, FLAG_FIXED_VERT, 1, 99999, 1,
			      99999, 0, 0,
			      EoGetX(dsk) + VRoot.w -
			      Conf.desks.dragbar_width, 0, EoGetY(dsk),
			      0, 0, 0, 0, 0, 1, 0, 1);
	  }
	else if (Conf.desks.dragdir == 2)
	  {
	     b = ButtonCreate("_DESKTOP_DESKRAY_DRAG_CONTROL", 2, ic4, ac,
			      NULL, NULL, 1, FLAG_FIXED_HORIZ, 1, 99999, 1,
			      99999, 0, 0, EoGetX(dsk), 0, EoGetY(dsk),
			      0, 0, 0, 0, 0, 1, 0, 1);
	  }
	else
	  {
	     b = ButtonCreate("_DESKTOP_DESKRAY_DRAG_CONTROL", 2, ic4, ac,
			      NULL, NULL, 1, FLAG_FIXED_HORIZ, 1, 99999, 1,
			      99999, 0, 0, EoGetX(dsk), 0,
			      EoGetY(dsk) + VRoot.h - Conf.desks.dragbar_width,
			      0, 0, 0, 0, 0, 1, 0, 1);
	  }
     }
#endif

   dsk->tag = b;
}

static void
DeskControlsDestroy(Desk * dsk, int id)
{
   Button            **blst;
   int                 num, i;

   blst = (Button **) ListItemTypeID(&num, LIST_TYPE_BUTTON, id);
   if (!blst)
      return;

   for (i = 0; i < num; i++)
      if (EobjGetDesk((EObj *) (blst[i])) == dsk)
	 ButtonDestroy(blst[i]);
   Efree(blst);
}

static void
DeskControlsShow(Desk * dsk, int id)
{
   Button            **blst;
   int                 num, i;

   blst = (Button **) ListItemTypeID(&num, LIST_TYPE_BUTTON, id);
   if (!blst)
      return;

   for (i = 0; i < num; i++)
      if (EobjGetDesk((EObj *) (blst[i])) == dsk)
	 ButtonShow(blst[i]);
   Efree(blst);
}

static void
DeskEventsConfigure(Desk * dsk, int mode)
{
   long                event_mask;
   XWindowAttributes   xwa;

   if (mode)
     {
	event_mask = dsk->event_mask;
     }
   else
     {
	EGetWindowAttributes(EoGetWin(dsk), &xwa);
	dsk->event_mask = xwa.your_event_mask | EDESK_EVENT_MASK;
	event_mask =
	   PropertyChangeMask | SubstructureRedirectMask |
	   ButtonPressMask | ButtonReleaseMask;
#if USE_COMPOSITE
	/* Handle ConfigureNotify's while sliding */
	event_mask |= SubstructureNotifyMask;
#endif
     }
   ESelectInput(EoGetWin(dsk), event_mask);
}

static void
DeskConfigure(Desk * dsk)
{
   Background        **lst, *bg;
   int                 num;
   unsigned int        rnd;

   DeskControlsCreate(dsk);
   DeskControlsShow(dsk, 1);

   bg = desks.bg[dsk->num];
   if (bg)
      bg = FindItem(bg, 0, LIST_FINDBY_POINTER, LIST_TYPE_BACKGROUND);
   if (!bg)
     {
	lst = (Background **) ListItemType(&num, LIST_TYPE_BACKGROUND);
	if (lst)
	  {
	     rnd = rand();
	     rnd %= num;
	     bg = lst[rnd];
	     Efree(lst);
	  }
     }
   DeskSetBg(dsk, bg, 0);

   if (dsk->num > 0)
     {
	EoMove(dsk, VRoot.w, 0);
	EoMap(dsk, 0);
     }

   ModulesSignal(ESIGNAL_DESK_ADDED, dsk);
}

static Desk        *
DeskCreate(int desk, int configure)
{
   Desk               *dsk;
   Window              win;
   char                buf[64];

   if (desk < 0 || desk >= ENLIGHTENMENT_CONF_NUM_DESKTOPS)
      return NULL;

   dsk = Ecalloc(1, sizeof(Desk));

   desks.desk[desk] = dsk;
   dsk->num = desk;
   desks.order[desk] = desk;

   win = (desk == 0) ? VRoot.win : None;
   Esnprintf(buf, sizeof(buf), "Desk-%d", desk);
   EobjInit(&dsk->o, EOBJ_TYPE_DESK, win, 0, 0, VRoot.w, VRoot.h, 0, buf);
   EventCallbackRegister(EoGetWin(dsk), 0, DeskHandleEvents, dsk);
   EoSetShadow(dsk, 0);
   if (desk == 0)
     {
	desks.current = dsk;
     }
   else
     {
	EoSetFloating(dsk, 1);
	EoSetLayer(dsk, 0);
#if 0				/* TBD */
	dsk->event_mask = EDESK_EVENT_MASK;
	DeskEventsConfigure(dsk, 1);
#endif
	/* Set the _XROOT... atoms so apps will find them even before the bg is set */
	HintsSetRootInfo(EoGetWin(dsk), None, 0);
     }

   HintsSetRootHints(EoGetWin(dsk));

   if (configure)
      DeskConfigure(dsk);

   return dsk;
}

static void
DeskDestroy(Desk * dsk)
{
   ModulesSignal(ESIGNAL_DESK_REMOVED, dsk);

   EventCallbackUnregister(EoGetWin(dsk), 0, DeskHandleEvents, dsk);

   DeskControlsDestroy(dsk, 1);
   DeskControlsDestroy(dsk, 2);

   if (dsk->bg)
      BackgroundDecRefcount(dsk->bg);

   EobjFini(&dsk->o);
   EDestroyWindow(EoGetWin(dsk));

   desks.desk[dsk->num] = NULL;
   Efree(dsk);
}

static void
DeskResize(int desk, int w, int h)
{
   Desk               *dsk;
   int                 x;

   dsk = _DeskGet(desk);

   if (dsk->num == 0)
     {
	EoSync(dsk);
     }
   else
     {
	x = (dsk->viewable) ? EoGetX(dsk) : VRoot.w;
	EoMoveResize(dsk, x, 0, w, h);
     }
   BackgroundPixmapFree(dsk->bg);
   DeskRefresh(dsk);
   DeskControlsDestroy(dsk, 1);
   DeskControlsCreate(dsk);
   DeskControlsShow(dsk, 1);
}

Desk               *
DeskGet(unsigned int desk)
{
   if (desk >= Conf.desks.num)
      return NULL;

   return _DeskGet(desk);
}

Desk               *
DeskGetRelative(Desk * dsk, int inc)
{
   unsigned int        desk;

   desk = (dsk) ? dsk->num : 0;
   desk += inc;
   desk %= Conf.desks.num;

   return _DeskGet(desk);
}

Background         *
DeskGetBackground(const Desk * dsk)
{
   return (dsk) ? dsk->bg : NULL;
}

void
DeskGetArea(const Desk * dsk, int *ax, int *ay)
{
   if (!dsk)
     {
	*ax = *ay = 0;
	return;
     }
   *ax = dsk->current_area_x;
   *ay = dsk->current_area_y;
}

void
DeskSetArea(Desk * dsk, int ax, int ay)
{
   if (!dsk)
      return;

   dsk->current_area_x = ax;
   dsk->current_area_y = ay;
}

int
DeskIsViewable(const Desk * dsk)
{
   return dsk->viewable;
}

void
DeskSetDirtyStack(Desk * dsk, EObj * eo)
{
   dsk->stack.dirty++;
   dsk->stack.latest = eo;
   if (EobjGetType(eo) == EOBJ_TYPE_EWIN)
      dsk->stack.update_client_list = 1;
   if (EventDebug(EDBUG_TYPE_STACKING))
      Eprintf("DeskSetDirtyStack %d (%d): %s\n", dsk->num, dsk->stack.dirty,
	      EobjGetName(eo));
}

void
DeskGetCurrentArea(int *ax, int *ay)
{
   DeskGetArea(desks.current, ax, ay);
}

void
DeskSetCurrentArea(int ax, int ay)
{
   DeskSetArea(desks.current, ax, ay);
}

unsigned int
DesksGetNumber(void)
{
   return Conf.desks.num;
}

Desk               *
DesksGetCurrent(void)
{
   return desks.current;
}

unsigned int
DesksGetCurrentNum(void)
{
   return desks.current->num;
}

void
DesksSetCurrent(Desk * dsk)
{
   if (!dsk)
      return;
   desks.current = dsk;
}

void
DesksClear(void)
{
   Desk               *dsk;
   unsigned int        i;

   for (i = 0; i < Conf.desks.num; i++)
     {
	dsk = _DeskGet(i);
	if (dsk->viewable)
	   EClearWindow(EoGetWin(dsk));
     }
}

void
DesksResize(int w, int h)
{
   unsigned int        i;

   for (i = 0; i < Conf.desks.num; i++)
      DeskResize(i, w, h);

   ModulesSignal(ESIGNAL_DESK_RESIZE, NULL);
}

void
DesksEventsConfigure(int mode)
{
   unsigned int        i;

   for (i = 0; i < Conf.desks.num; i++)
      DeskEventsConfigure(_DeskGet(i), mode);
}

static void
ChangeNumberOfDesktops(unsigned int quantity)
{
   unsigned int        i;
   int                 j, num;
   EWin               *const *lst;

   if (quantity >= ENLIGHTENMENT_CONF_NUM_DESKTOPS)
      quantity = ENLIGHTENMENT_CONF_NUM_DESKTOPS;

   if (quantity <= 0 || quantity == Conf.desks.num)
      return;

   for (i = quantity; i < Conf.desks.num; i++)
      DeskLower(i);

   if (quantity > Conf.desks.num)
     {
	while (Conf.desks.num < quantity)
	  {
	     Conf.desks.num++;
	     DeskCreate(Conf.desks.num - 1, 1);
	  }
     }
   else if (quantity < Conf.desks.num)
     {
	lst = EwinListGetAll(&num);
	for (j = 0; j < num; j++)
	  {
	     if (EoGetDeskNum(lst[j]) >= quantity)
		EwinMoveToDesktop(lst[j], _DeskGet(quantity - 1));
	  }

	while (Conf.desks.num > quantity)
	  {
	     DeskDestroy(_DeskGet(Conf.desks.num - 1));
	     Conf.desks.num--;
	  }
     }

   if (DesksGetCurrentNum() >= Conf.desks.num)
      DeskGotoNum(Conf.desks.num - 1);

   HintsSetDesktopConfig();

   autosave();
}

static void
DesksControlsCreate(void)
{
   unsigned int        i;

   for (i = 0; i < Conf.desks.num; i++)
      DeskControlsCreate(_DeskGet(i));
}

static void
DesksControlsDestroy(void)
{
   unsigned int        i;

   for (i = 0; i < Conf.desks.num; i++)
      DeskControlsDestroy(_DeskGet(i), 1);
}

static void
DesksControlsShow(void)
{
   unsigned int        i;

   for (i = 0; i < Conf.desks.num; i++)
      DeskControlsShow(_DeskGet(i), 1);
}

static void
DesksControlsRefresh(void)
{
   DesksControlsDestroy();
   DesksControlsCreate();
   DesksControlsShow();

   autosave();
}

#if 0				/* Unused */
static void
DeskShowTabs(void)
{
   Button            **blst;
   int                 num, i;

   blst = (Button **) ListItemTypeID(&num, LIST_TYPE_BUTTON, 2);
   if (blst)
     {
	for (i = 0; i < num; i++)
	   ButtonShow(blst[i]);
	Efree(blst);
     }
}

static void
DeskHideTabs(void)
{
   Button            **blst;
   int                 num, i;

   blst = (Button **) ListItemTypeID(&num, LIST_TYPE_BUTTON, 2);
   if (blst)
     {
	for (i = 0; i < num; i++)
	   ButtonHide(blst[i]);
	Efree(blst);
     }
}
#endif

static void
DeskShowButtons(void)
{
   Button            **blst;
   int                 i, num;

   blst = (Button **) ListItemTypeID(&num, LIST_TYPE_BUTTON, 0);
   if (blst)
     {
	for (i = 0; i < num; i++)
	  {
	     if (ButtonDoShowDefault(blst[i]))
		ButtonShow(blst[i]);
	  }
	Efree(blst);
     }
}

static void
MoveToDeskTop(unsigned int desk)
{
   int                 i, j;

   EobjListStackRaise(&desks.desk[desk]->o);

   j = -1;
   i = 0;
   while (j < 0 && i < (int)Conf.desks.num)
     {
	if (desks.order[i] == desk)
	   j = i;
	i++;
     }
   if (j < 0)
      return;
   if (j > 0)
     {
	for (i = j - 1; i >= 0; i--)
	   desks.order[i + 1] = desks.order[i];
	desks.order[0] = desk;
     }
}

static void
MoveToDeskBottom(unsigned int desk)
{
   int                 i, j;

   EobjListStackLower(&desks.desk[desk]->o);

   j = -1;
   i = 0;
   while (j < 0 && i < (int)Conf.desks.num)
     {
	if (desks.order[i] == desk)
	   j = i;
	i++;
     }
   if (j < 0)
      return;
   if (j < (int)Conf.desks.num - 1)
     {
	for (i = j; i < (int)Conf.desks.num - 1; i++)
	   desks.order[i] = desks.order[i + 1];
	desks.order[Conf.desks.num - 1] = desk;
     }
}

void
DeskRefresh(Desk * dsk)
{
   Background         *bg;

   if (!dsk || !dsk->viewable)
      return;

   if (EventDebug(EDBUG_TYPE_DESKS))
      Eprintf("DeskRefresh %d - %dx%d\n", dsk->num, EoGetW(dsk), EoGetH(dsk));

   bg = dsk->bg;
   if (!bg)
      return;

   if (BackgroundGetPixmap(bg) != None)
      return;

   BackgroundSet(bg, EoGetWin(dsk), EoGetW(dsk), EoGetH(dsk));
   HintsSetRootInfo(EoGetWin(dsk),
		    BackgroundGetPixmap(bg), BackgroundGetColor(bg));
}

void
DeskAssignBg(unsigned int desk, Background * bg)
{
   if (desk >= ENLIGHTENMENT_CONF_NUM_DESKTOPS)
      return;

   desks.bg[desk] = bg;
}

void
DeskSetBg(Desk * dsk, Background * bg, int refresh)
{
   if (!dsk)
      return;

   if (refresh)
      BackgroundPixmapFree(dsk->bg);

   if (bg && !strcmp(BackgroundGetName(bg), "NONE"))
      bg = NULL;

   if (dsk->bg != bg)
     {
	if (dsk->bg)
	   BackgroundDecRefcount(dsk->bg);
	if (bg)
	   BackgroundIncRefcount(bg);
     }

   dsk->bg = bg;

   if (dsk->viewable)
      DeskRefresh(dsk);

   ModulesSignal(ESIGNAL_BACKGROUND_CHANGE, dsk);
}

Desk               *
DesktopAt(int x, int y)
{
   Desk               *dsk;
   unsigned int        i;

   for (i = 0; i < Conf.desks.num; i++)
     {
	dsk = _DeskGet(desks.order[i]);
	if (x >= EoGetX(dsk) && x < (EoGetX(dsk) + VRoot.w) &&
	    y >= EoGetY(dsk) && y < (EoGetY(dsk) + VRoot.h))
	   return _DeskGet(desks.order[i]);
     }
   return _DeskGet(0);
}

static void
DesksStackingCheck(void)
{
   Desk               *dsk;
   unsigned int        i;

   for (i = 0; i < Conf.desks.num; i++)
     {
	dsk = _DeskGet(i);
	if (i && !dsk->viewable)
	   continue;
	if (!dsk->stack.dirty)
	   continue;
	DeskRestack(dsk);
     }
}

static void
DeskMove(Desk * dsk, int x, int y)
{
   Desk               *dd;
   unsigned int        i;
   EWin               *const *lst;
   int                 n, v, dx, dy;

   if (dsk->num <= 0)
      return;

   n = -1;
   i = 0;
   while (n < 0 && i < Conf.desks.num)
     {
	if (desks.order[i] == dsk->num)
	   n = i;
	i++;
     }
   if (n < 0)			/* Should not be possible */
      return;

   dx = x - EoGetX(dsk);
   dy = y - EoGetY(dsk);

   if (x == 0 && y == 0)
     {
	/* Desks below are obscured - touch and set unviewable */
	for (i = n + 1; i < Conf.desks.num; i++)
	  {
	     dd = _DeskGet(desks.order[i]);
	     if (dd->viewable)
		BackgroundTouch(dd->bg);
	     dd->viewable = 0;
	  }
     }
   else
     {
	v = dsk->viewable;

	for (i = n + 1; i < Conf.desks.num; i++)
	  {
	     dd = _DeskGet(desks.order[i]);
	     if (!dd->viewable && v)
	       {
		  dd->viewable = 1;
		  DeskRefresh(_DeskGet(desks.order[i]));
	       }
	     else if (dd->viewable && !v)
	       {
		  BackgroundTouch(dd->bg);
		  dd->viewable = 0;
	       }

	     if (EoGetX(dd) == 0 && EoGetY(dd) == 0)
		v = 0;
	  }
     }

   EoMove(dsk, x, y);

   if (dsk->tag)
      ButtonMoveRelative(dsk->tag, dx, dy);

   lst = EwinListGetAll(&n);
   for (i = 0; i < (unsigned int)n; i++)
      if (EoGetDesk(lst[i]) == dsk)
	 ICCCM_Configure(lst[i]);
}

static void
DeskHide(unsigned int desk)
{
   Desk               *dsk;

   if (desk <= 0 || desk >= Conf.desks.num)
      return;

   dsk = _DeskGet(desk);

   if (dsk->viewable)
      BackgroundTouch(dsk->bg);
   dsk->viewable = 0;
   EoMove(dsk, VRoot.w, 0);
}

static void
DeskEnter(Desk * dsk)
{
   int                 i;

   EGrabServer();

   dsk->viewable = 1;
   DeskRefresh(dsk);
   MoveToDeskTop(dsk->num);

   desks.previous = desks.current = dsk;

   if (dsk->num == 0)
     {
	for (i = Conf.desks.num - 1; i > 0; i--)
	   DeskHide(desks.order[i]);
     }

   EwinsMoveStickyToDesk(dsk);
   ButtonsMoveStickyToDesk(dsk);
   DesksStackingCheck();
   HintsSetCurrentDesktop();

   EUngrabServer();
}

void
DeskGotoNum(unsigned int desk)
{
   Desk               *dsk;

   if (Conf.desks.desks_wraparound)
      desk %= Conf.desks.num;
   if (desk >= Conf.desks.num || desk == desks.previous->num)
      return;

   dsk = _DeskGet(desk);
   DeskGoto(dsk);
}

void
DeskSwitchStart(void)
{
   FocusNewDeskBegin();

   /* we are about to flip desktops or areas - disable enter and leave events
    * temporarily */
   EwinsEventsConfigure(0);
   DesksEventsConfigure(0);
}

void
DeskSwitchDone(void)
{
   /* we flipped - re-enable enter and leave events */
   EwinsEventsConfigure(1);
   DesksEventsConfigure(1);

   FocusNewDesk();
}

void
DeskGoto(Desk * dsk)
{
   if (!dsk || dsk == desks.previous)

      if (EventDebug(EDBUG_TYPE_DESKS))
	 Eprintf("DeskGoto %d\n", dsk->num);

   ModulesSignal(ESIGNAL_DESK_SWITCH_START, NULL);

   ActionsSuspend();
   DeskSwitchStart();

   if (dsk->num > 0)
     {
	if (Conf.desks.slidein)
	  {
	     if (!dsk->viewable)
	       {
		  int                 x, y;

		  switch (Conf.desks.dragdir)
		    {
		    default:
		    case 0:
		       x = VRoot.w;
		       y = 0;
		       break;
		    case 1:
		       x = -VRoot.w;
		       y = 0;
		       break;
		    case 2:
		       x = 0;
		       y = VRoot.h;
		       break;
		    case 3:
		       x = 0;
		       y = -VRoot.h;
		       break;
		    }
		  DeskMove(dsk, x, y);
		  DeskEnter(dsk);
		  EobjSlideTo(&dsk->o, x, y, 0, 0, Conf.desks.slidespeed);
	       }
	     else
	       {
		  EobjSlideTo(&dsk->o, EoGetX(dsk), EoGetY(dsk), 0, 0,
			      Conf.desks.slidespeed);
		  DeskEnter(dsk);
	       }
	  }
	else
	  {
	     DeskEnter(dsk);
	  }
	DeskMove(dsk, 0, 0);
     }
   else
     {
	DeskEnter(dsk);
     }

   ActionsResume();
   DeskSwitchDone();

   ModulesSignal(ESIGNAL_DESK_SWITCH_DONE, NULL);
}

static void
UncoverDesktop(unsigned int desk)
{
   Desk               *dsk;

   if (desk >= Conf.desks.num)
      return;

   dsk = _DeskGet(desk);

   dsk->viewable = 1;
   DeskRefresh(dsk);
}

static void
DeskRaise(unsigned int desk)
{
   Desk               *dsk;

   if (desk >= Conf.desks.num)
      return;

   dsk = _DeskGet(desk);

   if (EventDebug(EDBUG_TYPE_DESKS))
      Eprintf("DeskRaise(%d) current=%d\n", desk, desks.current->num);

   DeskSwitchStart();
   DeskEnter(dsk);
   DeskSwitchDone();

   ModulesSignal(ESIGNAL_DESK_SWITCH_DONE, NULL);

   ESync();
}

static void
DeskLower(unsigned int desk)
{
   if ((desk <= 0) || (desk >= Conf.desks.num))
      return;

   DeskSwitchStart();
   MoveToDeskBottom(desk);

   if (EventDebug(EDBUG_TYPE_DESKS))
      Eprintf("DeskLower(%d) %d -> %d\n", desk, desks.current->num,
	      desks.order[0]);

   desks.previous = desks.current = DeskGet(desks.order[0]);

   EGrabServer();

   UncoverDesktop(desks.order[0]);
   DeskHide(desk);

   EwinsMoveStickyToDesk(desks.current);
   ButtonsMoveStickyToDesk(desks.current);
   DesksStackingCheck();
   DeskSwitchDone();
   if (Mode.mode == MODE_NONE)
      ModulesSignal(ESIGNAL_DESK_SWITCH_DONE, NULL);
   HintsSetCurrentDesktop();

   EUngrabServer();
   ESync();
}

#if 0				/* Unused */
void
DeskShow(int desk)
{
   Desk               *dsk;
   int                 i;

   if (desk < 0 || desk >= Conf.desks.num)
      return;

   dsk = _DeskGet(desk);

   dsk->viewable = 1;
   DeskRefresh(dsk);
   MoveToDeskTop(desk);

   if (desk == 0)
     {
	for (i = Conf.desks.num - 1; i > 0; i--)
	   DeskHide(desks.order[i]);
     }
}
#endif

static void
DeskRestackSimple(Desk * dsk)
{
   EObj               *const *lst, *eo;
   int                 i, num;
   XWindowChanges      xwc;
   unsigned int        value_mask;

   eo = dsk->stack.latest;
   eo->stacked = 1;

   if (EventDebug(EDBUG_TYPE_STACKING))
      Eprintf("DeskRestackSimple %#lx %s\n", EobjGetWin(eo), EobjGetName(eo));

   lst = EobjListStackGetForDesk(&num, dsk);
   if (num < 2)
      return;

   for (i = 0; i < num; i++)
      if (lst[i] == eo)
	 break;
   if (i >= num)
      return;

   if (i < num - 1)
     {
	xwc.stack_mode = Above;
	xwc.sibling = EobjGetWin(lst[i + 1]);
     }
   else
     {
	xwc.stack_mode = Below;
	xwc.sibling = EobjGetWin(lst[i - 1]);
     }
   value_mask = CWSibling | CWStackMode;
   if (EventDebug(EDBUG_TYPE_STACKING))
      Eprintf("DeskRestackSimple %#10lx %s %#10lx\n", EobjGetWin(eo),
	      (xwc.stack_mode == Above) ? "Above" : "Below", xwc.sibling);
   XConfigureWindow(disp, EobjGetWin(eo), value_mask, &xwc);
}

#define _APPEND_TO_WIN_LIST(win) \
  { \
     wl = Erealloc(wl, ++tot * sizeof(Window)); \
     wl[tot - 1] = win; \
  }
void
DeskRestack(Desk * dsk)
{
   Window             *wl;
   int                 i, num, tot;
   EObj               *const *lst, *eo;

   /* Special case if only one window needs restacking */
   if (dsk->stack.dirty == 1)
     {
	DeskRestackSimple(dsk);
	goto done;
     }

   /* Build the window stack, top to bottom */
   tot = 0;
   wl = NULL;
   lst = EobjListStackGetForDesk(&num, dsk);

   /* Normal objects */
   for (i = 0; i < num; i++)
     {
	eo = lst[i];
	_APPEND_TO_WIN_LIST(eo->win);
	eo->stacked = 1;
     }

   if (EventDebug(EDBUG_TYPE_STACKING))
     {
	Eprintf("DeskRestack %d (%d):\n", dsk->num, dsk->stack.dirty);
	for (i = 0; i < tot; i++)
	   Eprintf(" win=%#10lx parent=%#10lx\n", wl[i],
		   EWindowGetParent(wl[i]));
     }

   XRestackWindows(disp, wl, tot);

   if (wl)
      Efree(wl);

 done:
   if (dsk->stack.update_client_list)
     {
	dsk->stack.update_client_list = 0;
	HintsSetClientStacking();
     }
   dsk->stack.dirty = 0;
   dsk->stack.latest = NULL;
}

void
DeskGotoByEwin(EWin * ewin)
{
   if (EoIsSticky(ewin) || EoIsFloating(ewin))
      return;

   DeskGoto(EoGetDesk(ewin));
   SetCurrentArea(ewin->area_x, ewin->area_y);
}

static char         sentpress = 0;

static void
ButtonProxySendEvent(XEvent * ev)
{
   if (Mode.button_proxy_win)
      XSendEvent(disp, Mode.button_proxy_win, False, SubstructureNotifyMask,
		 ev);
}

static void
DeskDragStart(int desk)
{
   Desk               *dsk;

   dsk = _DeskGet(desk);

   desks.drag_x0 = Mode.events.x - EoGetX(dsk);
   desks.drag_y0 = Mode.events.y - EoGetY(dsk);

   Mode.mode = MODE_DESKDRAG;
}

static void
DeskDragEnd(Desk * dsk __UNUSED__)
{
   Mode.mode = MODE_NONE;
}

static void
DeskDragMotion(Desk * dsk)
{
   int                 x, y;

   x = Mode.events.x - desks.drag_x0;
   y = Mode.events.y - desks.drag_y0;

   switch (Conf.desks.dragdir)
     {
     case 0:
	if (x < 0)
	   x = 0;
	y = 0;
	break;
     case 1:
	if (x > 0)
	   x = 0;
	y = 0;
	break;
     case 2:
	x = 0;
	if (y < 0)
	   y = 0;
	break;
     case 3:
	x = 0;
	if (y > 0)
	   y = 0;
	break;
     default:
	break;
     }
   DeskMove(dsk, x, y);
}

static void
DeskButtonCallback(EObj * eo, XEvent * ev, ActionClass * ac)
{
   Desk               *dsk;

   if (Mode.mode != MODE_DESKDRAG)
     {
	if (ac)
	   ActionclassEvent(ac, ev, NULL);
	return;
     }

   dsk = (Desk *) eo;
   switch (ev->type)
     {
     case ButtonRelease:
	DeskDragEnd(dsk);
	break;
     case MotionNotify:
	DeskDragMotion(dsk);
	break;
     }
}

static int
DeskCheckAction(Desk * dsk __UNUSED__, XEvent * ev)
{
   ActionClass        *ac;

   ac = FindItem("DESKBINDINGS", 0, LIST_FINDBY_NAME, LIST_TYPE_ACLASS);
   if (!ac)
      return 0;

   return ActionclassEvent(ac, ev, NULL);
}

static void
DeskEventButtonPress(Desk * dsk, XEvent * ev)
{
   /* Don't handle desk bindings while doing stuff */
   if (Mode.mode)
      return;

   GrabPointerRelease();

   if (!DeskCheckAction(dsk, ev))
      ButtonProxySendEvent(ev);
}

static void
DeskEventButtonRelease(Desk * dsk, XEvent * ev)
{
   /* Don't handle desk bindings while doing stuff */
   if (Mode.mode)
      return;

   if (sentpress)
     {
	/* We never get here? */
	sentpress = 0;
	ButtonProxySendEvent(ev);
     }

   DeskCheckAction(dsk, ev);
}

static void
DeskRootResize(int root, int w, int h)
{
   if (EventDebug(EDBUG_TYPE_DESKS))
      Eprintf("DeskRootResize %d %dx%d\n", root, w, h);

   if (root)
     {
#if 0
	RRoot.w = DisplayWidth(disp, RRoot.scr);
	RRoot.h = DisplayHeight(disp, RRoot.scr);

	if (w != RRoot.w || h != RRoot.h)
	   Eprintf
	      ("DeskRootResize (root): Screen size mismatch: root=%dx%d event=%dx%d\n",
	       RRoot.w, RRoot.h, w, h);
#endif
	RRoot.w = w;
	RRoot.h = h;
     }

   if (w == VRoot.w && h == VRoot.h)
      return;

   EWindowSync(VRoot.win);
   VRoot.w = w;
   VRoot.h = h;

   DesksResize(w, h);
}

static ActionClass *
DeskGetAclass(void *data __UNUSED__)
{
   return FindItem("DESKBINDINGS", 0, LIST_FINDBY_NAME, LIST_TYPE_ACLASS);
}

static void
DeskHandleTooltip(Desk * dsk, XEvent * ev)
{
   switch (ev->type)
     {
     case ButtonPress:
     case LeaveNotify:
	TooltipsSetPending(1, NULL, NULL);
	break;

     case ButtonRelease:
	if (ev->xbutton.subwindow == None)
	   goto do_set_pending;
	break;

     case MotionNotify:
	if (ev->xmotion.subwindow == None)
	   goto do_set_pending;
	break;
     case EnterNotify:
	if (ev->xcrossing.mode == NotifyNormal &&
	    ev->xcrossing.detail == NotifyInferior)
	   goto do_set_pending;
	break;

      do_set_pending:
	TooltipsSetPending(1, DeskGetAclass, dsk);
	break;
     }
}

static void
DeskHandleEvents(XEvent * ev, void *prm)
{
   Desk               *dsk = (Desk *) prm;

   switch (ev->type)
     {
     case ButtonPress:
	DeskEventButtonPress(dsk, ev);
	break;
     case ButtonRelease:
	DeskEventButtonRelease(dsk, ev);
	break;

     case EnterNotify:
	FocusHandleEnter(NULL, ev);
	break;
     case LeaveNotify:
	FocusHandleLeave(NULL, ev);
	break;

     case MotionNotify:
	/* Motion over desk buttons doesn't go here - We probably don't care much. */
	DesksSetCurrent(DesktopAt(Mode.events.x, Mode.events.y));
	break;

     case ConfigureNotify:
	if (ev->xconfigure.window == VRoot.win)
	   DeskRootResize(0, ev->xconfigure.width, ev->xconfigure.height);
	break;

#if USE_XRANDR
     case EX_EVENT_SCREEN_CHANGE_NOTIFY:
	{
	   XRRScreenChangeNotifyEvent *rrev = (XRRScreenChangeNotifyEvent *) ev;

	   DeskRootResize(1, rrev->width, rrev->height);
	}
	break;
#endif
     }

   DeskHandleTooltip(dsk, ev);
}

/* Settings */

static void
DeskDragdirSet(const char *params)
{
   Desk               *dsk;
   unsigned int        i;
   int                 pd;

   pd = Conf.desks.dragdir;

   if (params && params[0])
      Conf.desks.dragdir = atoi(params);
   else
     {
	Conf.desks.dragdir++;
	if (Conf.desks.dragdir > 3)
	   Conf.desks.dragdir = 0;
     }

   if (pd == Conf.desks.dragdir)
      return;

   for (i = 1; i < Conf.desks.num; i++)
     {
	dsk = _DeskGet(i);
	EoMove(dsk, (dsk->viewable) ? 0 : VRoot.w, 0);
     }
   DesksControlsRefresh();
}

static void
DeskDragbarOrderSet(const char *params)
{
   int                 pd;

   pd = Conf.desks.dragbar_ordering;

   if (params && params[0])
      Conf.desks.dragbar_ordering = atoi(params);
   else
     {
	Conf.desks.dragbar_ordering++;
	if (Conf.desks.dragbar_ordering > 5)
	   Conf.desks.dragbar_ordering = 0;
     }

   if (pd == Conf.desks.dragbar_ordering)
      return;

   DesksControlsRefresh();
}

#if 0				/* FIXME */

static int
doDragbarWidthSet(EWin * edummy, const char *params)
{
   int                 pd;
   Button             *b;

   pd = Conf.desks.dragbar_width;
   if (params)
      Conf.desks.dragbar_width = atoi(params);

   if (pd != Conf.desks.dragbar_width)
     {
	DesksControlsRefresh();
     }
   return 0;
}

static int
doDragbarLengthSet(EWin * edummy, const char *params)
{
   int                 pd;
   Button             *b;

   pd = Conf.desks.dragbar_length;
   if (params)
      Conf.desks.dragbar_length = atoi(params);

   if (pd != Conf.desks.dragbar_length)
     {
	DesksControlsRefresh();
     }
   return 0;
}

static int
doDeskray(EWin * edummy, const char *params)
{
   if (params)
     {
	if (!atoi(params))
	  {
	     DeskHideTabs();
	     Conf.deskmode = MODE_NONE;
	  }
	else
	  {
	     Conf.deskmode = MODE_DESKRAY;
	     DeskShowTabs();
	  }
     }
   else
     {
	if (Conf.deskmode == MODE_DESKRAY)
	  {
	     DeskHideTabs();
	     Conf.deskmode = MODE_NONE;
	  }
	else
	  {
	     Conf.deskmode = MODE_DESKRAY;
	     DeskShowTabs();
	  }
     }
   return 0;
}
#endif

static void
DesksInit(void)
{
   unsigned int        i;

   memset(&desks, 0, sizeof(desks));

   desks.previous = NULL;

   for (i = 0; i < Conf.desks.num; i++)
      DeskCreate(i, 0);

   SetAreaSize(Conf.desks.areas_nx, Conf.desks.areas_ny);

   /* Retreive stuff from last time we were loaded if we're restarting */
   EHintsGetDeskInfo();

   HintsSetDesktopConfig();
}

static void
DesksConfigure(void)
{
   unsigned int        i;

   for (i = 0; i < Conf.desks.num; i++)
      DeskConfigure(_DeskGet(i));

   UncoverDesktop(0);
}

/*
 * Desktops Module
 */

static void
DesksSighan(int sig, void *prm __UNUSED__)
{
   switch (sig)
     {
     case ESIGNAL_INIT:
	DesksInit();
	break;

     case ESIGNAL_CONFIGURE:
	DesksConfigure();
	break;

     case ESIGNAL_START:
	/* Draw all the buttons that belong on the desktop */
	DeskShowButtons();
	break;

     case ESIGNAL_IDLE:
	DesksStackingCheck();
	break;
     }
}

/*
 * Dialodgs
 */
static int          tmp_desktops;
static DItem       *tmp_desk_text;
static Dialog      *tmp_desk_dialog;
static char         tmp_desktop_slide;
static int          tmp_desktop_slide_speed;
static char         tmp_desktop_wraparound;
static char         tmp_dragbar;
static int          tmp_dragdir;

static void
CB_ConfigureDesktops(Dialog * d __UNUSED__, int val, void *data __UNUSED__)
{
   if (val < 2)
     {
	ChangeNumberOfDesktops(tmp_desktops);
	Conf.desks.slidein = tmp_desktop_slide;
	Conf.desks.slidespeed = tmp_desktop_slide_speed;
	Conf.desks.desks_wraparound = tmp_desktop_wraparound;

	if ((Conf.desks.dragdir != tmp_dragdir) ||
	    ((tmp_dragbar) && (Conf.desks.dragbar_width < 1)) ||
	    ((!tmp_dragbar) && (Conf.desks.dragbar_width > 0)))
	  {
	     if (tmp_dragbar)
		Conf.desks.dragbar_width = 16;
	     else
		Conf.desks.dragbar_width = 0;
	     Conf.desks.dragdir = tmp_dragdir;
	     DesksControlsRefresh();
	  }
     }
}

static void
CB_DesktopDisplayRedraw(Dialog * d __UNUSED__, int val, void *data)
{
   static char         called = 0;
   DItem              *di;
   static Window       win, wins[ENLIGHTENMENT_CONF_NUM_DESKTOPS];
   int                 i;
   int                 w, h;
   static int          prev_desktops = -1;
   char                s[64];
   ImageClass         *ic;

   if (val == 1)
      called = 0;

   if ((val != 1) && (prev_desktops == tmp_desktops))
      return;

   prev_desktops = tmp_desktops;
   di = (DItem *) data;
   win = DialogItemAreaGetWindow(di);
   DialogItemAreaGetSize(di, &w, &h);

   if (!called)
     {
	ic = ImageclassFind("SETTINGS_DESKTOP_AREA", 0);
	if (ic)
	   ImageclassApply(ic, win, w, h, 0, 0, STATE_NORMAL, 0, ST_UNKNWN);
	for (i = 0; i < ENLIGHTENMENT_CONF_NUM_DESKTOPS; i++)
	   wins[i] = 0;
	called = 1;
     }

   for (i = 0; i < tmp_desktops; i++)
     {
	if (!wins[i])
	  {
	     Background         *bg;
	     Pixmap              pmap;

	     wins[i] = ECreateWindow(win, 0, 0, 64, 48, 0);
	     ESetWindowBorderWidth(wins[i], 1);
	     pmap = ECreatePixmap(wins[i], 64, 48, VRoot.depth);
	     ESetWindowBackgroundPixmap(wins[i], pmap);

	     bg = DeskGetBackground(DeskGet(i));
	     if (bg)
		BackgroundApply(bg, pmap, 64, 48, 0);
	     else
	       {
		  ic = ImageclassFind("SETTINGS_DESKTOP_AREA", 0);
		  if (ic)
		     ImageclassApply(ic, wins[i], 64, 48, 0, 0, STATE_NORMAL, 0,
				     ST_UNKNWN);
	       }

	     EFreePixmap(pmap);
	  }
     }

   for (i = tmp_desktops - 1; i >= 0; i--)
     {
	int                 num;

	num = tmp_desktops - 1;
	if (num < 1)
	   num = 1;
	EMoveWindow(wins[i], (i * (w - 64 - 2)) / num,
		    (i * (h - 48 - 2)) / num);
	ERaiseWindow(wins[i]);
	EMapWindow(wins[i]);
     }

   for (i = tmp_desktops; i < (int)Conf.desks.num; i++)
     {
	if (!wins[i])
	   continue;
	EUnmapWindow(wins[i]);
     }

   if (tmp_desktops > 1)
      Esnprintf(s, sizeof(s), _("%i Desktops"), tmp_desktops);
   else
      Esnprintf(s, sizeof(s), _("%i Desktop"), tmp_desktops);
   DialogItemSetText(tmp_desk_text, s);
   DialogDrawItems(tmp_desk_dialog, tmp_desk_text, 0, 0, 99999, 99999);
}

void
SettingsDesktops(void)
{
   Dialog             *d;
   DItem              *table, *di, *area, *slider, *radio;
   char                s[64];

   if ((d =
	FindItem("CONFIGURE_DESKTOPS", 0, LIST_FINDBY_NAME, LIST_TYPE_DIALOG)))
     {
	SoundPlay("SOUND_SETTINGS_ACTIVE");
	ShowDialog(d);
	return;
     }
   SoundPlay("SOUND_SETTINGS_DESKTOPS");

   tmp_desktops = Conf.desks.num;
   tmp_desktop_slide = Conf.desks.slidein;
   tmp_desktop_slide_speed = Conf.desks.slidespeed;
   tmp_desktop_wraparound = Conf.desks.desks_wraparound;
   if (Conf.desks.dragbar_width < 1)
      tmp_dragbar = 0;
   else
      tmp_dragbar = 1;
   tmp_dragdir = Conf.desks.dragdir;

   d = tmp_desk_dialog = DialogCreate("CONFIGURE_DESKTOPS");
   DialogSetTitle(d, _("Multiple Desktop Settings"));

   table = DialogInitItem(d);
   DialogItemTableSetOptions(table, 2, 0, 0, 0);

   if (Conf.dialogs.headers)
     {
	di = DialogAddItem(table, DITEM_IMAGE);
	DialogItemSetPadding(di, 2, 2, 2, 2);
	DialogItemImageSetFile(di, "pix/desktops.png");

	di = DialogAddItem(table, DITEM_TEXT);
	DialogItemSetPadding(di, 2, 2, 2, 2);
	DialogItemSetFill(di, 1, 0);
	DialogItemSetText(di,
			  _("Enlightenment Multiple Desktop\n"
			    "Settings Dialog\n"));

	di = DialogAddItem(table, DITEM_SEPARATOR);
	DialogItemSetColSpan(di, 2);
	DialogItemSetPadding(di, 2, 2, 2, 2);
	DialogItemSetFill(di, 1, 0);
	DialogItemSeparatorSetOrientation(di, 0);
     }

   di = DialogAddItem(table, DITEM_TEXT);
   DialogItemSetPadding(di, 2, 2, 2, 2);
   DialogItemSetFill(di, 1, 0);
   DialogItemSetColSpan(di, 2);
   DialogItemSetText(di, _("Number of virtual desktops:\n"));

   di = tmp_desk_text = DialogAddItem(table, DITEM_TEXT);
   DialogItemSetPadding(di, 2, 2, 2, 2);
   DialogItemSetFill(di, 1, 0);
   DialogItemSetColSpan(di, 2);
   if (tmp_desktops > 1)
      Esnprintf(s, sizeof(s), _("%i Desktops"), tmp_desktops);
   else
      Esnprintf(s, sizeof(s), _("%i Desktop"), tmp_desktops);
   DialogItemSetText(di, s);

   di = slider = DialogAddItem(table, DITEM_SLIDER);
   DialogItemSetPadding(di, 2, 2, 2, 2);
   DialogItemSetFill(di, 1, 0);
   DialogItemSliderSetBounds(di, 1, 32);
   DialogItemSliderSetUnits(di, 1);
   DialogItemSliderSetJump(di, 1);
   DialogItemSetColSpan(di, 2);
   DialogItemSliderSetValPtr(di, &tmp_desktops);

   di = area = DialogAddItem(table, DITEM_AREA);
   DialogItemSetPadding(di, 2, 2, 2, 2);
   DialogItemSetFill(di, 1, 0);
   DialogItemSetColSpan(di, 2);
   DialogItemSetPadding(di, 2, 2, 2, 2);
   DialogItemAreaSetSize(di, 128, 96);

   di = DialogAddItem(table, DITEM_SEPARATOR);
   DialogItemSetColSpan(di, 2);
   DialogItemSetPadding(di, 2, 2, 2, 2);
   DialogItemSetFill(di, 1, 0);
   DialogItemSeparatorSetOrientation(di, 0);

   di = DialogAddItem(table, DITEM_CHECKBUTTON);
   DialogItemSetPadding(di, 2, 2, 2, 2);
   DialogItemSetFill(di, 1, 0);
   DialogItemSetColSpan(di, 2);
   DialogItemSetText(di, _("Slide desktops around when changing"));
   DialogItemCheckButtonSetPtr(di, &tmp_desktop_slide);

   di = DialogAddItem(table, DITEM_TEXT);
   DialogItemSetPadding(di, 2, 2, 2, 2);
   DialogItemSetFill(di, 1, 0);
   DialogItemSetColSpan(di, 2);
   DialogItemSetAlign(di, 1024, 512);
   DialogItemSetText(di, _("Desktop Slide speed:\n"));

   di = DialogAddItem(table, DITEM_SLIDER);
   DialogItemSetColSpan(di, 2);
   DialogItemSetPadding(di, 2, 2, 2, 2);
   DialogItemSetFill(di, 1, 0);
   DialogItemSliderSetBounds(di, 0, 20000);
   DialogItemSliderSetUnits(di, 500);
   DialogItemSliderSetJump(di, 1000);
   DialogItemSliderSetValPtr(di, &tmp_desktop_slide_speed);

   di = DialogAddItem(table, DITEM_SEPARATOR);
   DialogItemSetPadding(di, 2, 2, 2, 2);
   DialogItemSetFill(di, 1, 0);
   DialogItemSetColSpan(di, 2);
   DialogItemSeparatorSetOrientation(di, 0);

   di = DialogAddItem(table, DITEM_CHECKBUTTON);
   DialogItemSetPadding(di, 2, 2, 2, 2);
   DialogItemSetFill(di, 1, 0);
   DialogItemSetColSpan(di, 2);
   DialogItemSetText(di, _("Wrap desktops around"));
   DialogItemCheckButtonSetPtr(di, &tmp_desktop_wraparound);

   di = DialogAddItem(table, DITEM_SEPARATOR);
   DialogItemSetPadding(di, 2, 2, 2, 2);
   DialogItemSetFill(di, 1, 0);
   DialogItemSetColSpan(di, 2);
   DialogItemSeparatorSetOrientation(di, 0);

   di = DialogAddItem(table, DITEM_CHECKBUTTON);
   DialogItemSetPadding(di, 2, 2, 2, 2);
   DialogItemSetFill(di, 1, 0);
   DialogItemSetColSpan(di, 2);
   DialogItemSetText(di, _("Display desktop dragbar"));
   DialogItemCheckButtonSetPtr(di, &tmp_dragbar);

   di = DialogAddItem(table, DITEM_TEXT);
   DialogItemSetColSpan(di, 2);
   DialogItemSetPadding(di, 2, 2, 2, 2);
   DialogItemSetFill(di, 1, 0);
   DialogItemSetAlign(di, 0, 512);
   DialogItemSetText(di, _("Drag bar position:"));

   radio = di = DialogAddItem(table, DITEM_RADIOBUTTON);
   DialogItemSetColSpan(di, 2);
   DialogItemSetPadding(di, 2, 2, 2, 2);
   DialogItemSetFill(di, 1, 0);
   DialogItemSetText(di, _("Top"));
   DialogItemRadioButtonSetFirst(di, radio);
   DialogItemRadioButtonGroupSetVal(di, 2);

   di = DialogAddItem(table, DITEM_RADIOBUTTON);
   DialogItemSetColSpan(di, 2);
   DialogItemSetPadding(di, 2, 2, 2, 2);
   DialogItemSetFill(di, 1, 0);
   DialogItemSetText(di, _("Bottom"));
   DialogItemRadioButtonSetFirst(di, radio);
   DialogItemRadioButtonGroupSetVal(di, 3);

   di = DialogAddItem(table, DITEM_RADIOBUTTON);
   DialogItemSetColSpan(di, 2);
   DialogItemSetPadding(di, 2, 2, 2, 2);
   DialogItemSetFill(di, 1, 0);
   DialogItemSetText(di, _("Left"));
   DialogItemRadioButtonSetFirst(di, radio);
   DialogItemRadioButtonGroupSetVal(di, 0);

   di = DialogAddItem(table, DITEM_RADIOBUTTON);
   DialogItemSetColSpan(di, 2);
   DialogItemSetPadding(di, 2, 2, 2, 2);
   DialogItemSetFill(di, 1, 0);
   DialogItemSetText(di, _("Right"));
   DialogItemRadioButtonSetFirst(di, radio);
   DialogItemRadioButtonGroupSetVal(di, 1);
   DialogItemRadioButtonGroupSetValPtr(radio, &tmp_dragdir);

   di = DialogAddItem(table, DITEM_SEPARATOR);
   DialogItemSetColSpan(di, 2);
   DialogItemSetPadding(di, 2, 2, 2, 2);
   DialogItemSetFill(di, 1, 0);
   DialogItemSeparatorSetOrientation(di, 0);

   DialogAddButton(d, _("OK"), CB_ConfigureDesktops, 1, DIALOG_BUTTON_OK);
   DialogAddButton(d, _("Apply"), CB_ConfigureDesktops, 0, DIALOG_BUTTON_APPLY);
   DialogAddButton(d, _("Close"), CB_ConfigureDesktops, 1, DIALOG_BUTTON_CLOSE);
   DialogSetExitFunction(d, CB_ConfigureDesktops, 2);
   DialogBindKey(d, "Escape", DialogCallbackClose, 0);
   DialogBindKey(d, "Return", CB_ConfigureDesktops, 0);
   ShowDialog(d);
   DialogItemSetCallback(slider, CB_DesktopDisplayRedraw, 0, (void *)area);
   CB_DesktopDisplayRedraw(d, 1, area);
}

static int          tmp_area_x;
static int          tmp_area_y;
static int          tmp_edge_resist;
static char         tmp_edge_flip;
static DItem       *tmp_area_text;
static Dialog      *tmp_area_dialog;
static char         tmp_area_wraparound;

static void
CB_ConfigureAreas(Dialog * d __UNUSED__, int val, void *data __UNUSED__)
{
   if (val < 2)
     {
	SetNewAreaSize(tmp_area_x, 9 - tmp_area_y);
	Conf.desks.areas_wraparound = tmp_area_wraparound;
	if (tmp_edge_flip)
	  {
	     if (tmp_edge_resist < 1)
		tmp_edge_resist = 1;
	     Conf.edge_flip_resistance = tmp_edge_resist;
	  }
	else
	   Conf.edge_flip_resistance = 0;
	EdgeWindowsShow();
     }
   autosave();
}

static void
CB_AreaDisplayRedraw(Dialog * d __UNUSED__, int val, void *data)
{
   static int          prev_ax = 0, prev_ay = 0;
   static char         called = 0;
   static Window       awin;
   char                s[64];
   DItem              *di;
   Window              win;
   int                 w, h;

   if (val == 1)
      called = 0;

   if ((val != 1) && ((prev_ax == tmp_area_x) && (prev_ay == tmp_area_y)))
      return;

   prev_ax = tmp_area_x;
   prev_ay = tmp_area_y;

   di = (DItem *) data;
   win = DialogItemAreaGetWindow(di);
   DialogItemAreaGetSize(di, &w, &h);

   if (!called)
     {
	ImageClass         *ic;

	ic = ImageclassFind("SETTINGS_AREA_AREA", 0);
	if (ic)
	   ImageclassApply(ic, win, w, h, 0, 0, STATE_NORMAL, 0, ST_UNKNWN);
	awin = ECreateWindow(win, 0, 0, 18, 14, 0);
	ic = ImageclassFind("SETTINGS_AREADESK_AREA", 0);
	if (ic)
	  {
	     Pixmap              pmap;

	     pmap = ImageclassApplySimple(ic, awin, None, STATE_NORMAL,
					  0, 0, 18, 14);
	     ESetWindowBackgroundPixmap(awin, pmap);
	     EFreePixmap(pmap);
	  }
	EClearWindow(awin);
	called = 1;
     }
   EMoveResizeWindow(awin, ((w / 2) - (9 * tmp_area_x)),
		     ((h / 2) - (7 * (9 - tmp_area_y))), 18 * tmp_area_x,
		     14 * (9 - tmp_area_y));
   EMapWindow(awin);

   if ((tmp_area_x > 1) || ((9 - tmp_area_y) > 1))
      Esnprintf(s, sizeof(s), _("%i x %i\nScreens in size"), tmp_area_x,
		9 - tmp_area_y);
   else
      Esnprintf(s, sizeof(s), _("1\nScreen in size"));
   DialogItemSetText(tmp_area_text, s);
   DialogDrawItems(tmp_area_dialog, tmp_area_text, 0, 0, 99999, 99999);
}

void
SettingsArea(void)
{
   Dialog             *d;
   DItem              *table, *di, *area, *slider, *slider2, *table2;
   char                s[64];

   if ((d = FindItem("CONFIGURE_AREA", 0, LIST_FINDBY_NAME, LIST_TYPE_DIALOG)))
     {
	SoundPlay("SOUND_SETTINGS_ACTIVE");
	ShowDialog(d);
	return;
     }
   SoundPlay("SOUND_SETTINGS_AREA");

   tmp_area_wraparound = Conf.desks.areas_wraparound;
   tmp_edge_resist = Conf.edge_flip_resistance;
   if (tmp_edge_resist == 0)
      tmp_edge_flip = 0;
   else
      tmp_edge_flip = 1;
   GetAreaSize(&tmp_area_x, &tmp_area_y);
   tmp_area_y = 9 - tmp_area_y;

   d = tmp_area_dialog = DialogCreate("CONFIGURE_AREA");
   DialogSetTitle(d, _("Virtual Desktop Settings"));

   table = DialogInitItem(d);
   DialogItemTableSetOptions(table, 1, 0, 0, 0);

   if (Conf.dialogs.headers)
     {
	table2 = DialogAddItem(table, DITEM_TABLE);
	DialogItemTableSetOptions(table2, 2, 0, 0, 0);

	di = DialogAddItem(table2, DITEM_IMAGE);
	DialogItemSetPadding(di, 2, 2, 2, 2);
	DialogItemImageSetFile(di, "pix/areas.png");

	di = DialogAddItem(table2, DITEM_TEXT);
	DialogItemSetPadding(di, 2, 2, 2, 2);
	DialogItemSetFill(di, 1, 0);
	DialogItemSetText(di,
			  _("Enlightenment Virtual Desktop\n"
			    "Settings Dialog\n"));

	di = DialogAddItem(table, DITEM_SEPARATOR);
	DialogItemSetPadding(di, 2, 2, 2, 2);
	DialogItemSetFill(di, 1, 0);
	DialogItemSeparatorSetOrientation(di, 0);
     }

   di = DialogAddItem(table, DITEM_TEXT);
   DialogItemSetPadding(di, 2, 2, 2, 2);
   DialogItemSetFill(di, 1, 0);
   DialogItemSetText(di, _("Virtual Desktop size:\n"));

   di = tmp_area_text = DialogAddItem(table, DITEM_TEXT);
   DialogItemSetPadding(di, 2, 2, 2, 2);
   DialogItemSetFill(di, 1, 0);
   if ((tmp_area_x > 1) || (tmp_area_y > 1))
      Esnprintf(s, sizeof(s), _("%i x %i\nScreens in size"), tmp_area_x,
		9 - tmp_area_y);
   else
      Esnprintf(s, sizeof(s), _("1\nScreen in size"));
   DialogItemSetText(di, s);

   table2 = DialogAddItem(table, DITEM_TABLE);
   DialogItemTableSetOptions(table2, 2, 0, 0, 0);

   di = DialogAddItem(table2, DITEM_NONE);

   di = slider = DialogAddItem(table2, DITEM_SLIDER);
   DialogItemSliderSetMinLength(di, 10);
   DialogItemSetPadding(di, 2, 2, 2, 2);
   DialogItemSetFill(di, 1, 0);
   DialogItemSliderSetBounds(di, 1, 8);
   DialogItemSliderSetUnits(di, 1);
   DialogItemSliderSetJump(di, 1);
   DialogItemSliderSetValPtr(di, &tmp_area_x);

   di = slider2 = DialogAddItem(table2, DITEM_SLIDER);
   DialogItemSliderSetMinLength(di, 10);
   DialogItemSliderSetOrientation(di, 0);
   DialogItemSetPadding(di, 2, 2, 2, 2);
   DialogItemSetFill(di, 0, 1);
   DialogItemSliderSetBounds(di, 1, 8);
   DialogItemSliderSetUnits(di, 1);
   DialogItemSliderSetJump(di, 1);
   DialogItemSliderSetValPtr(di, &tmp_area_y);

   di = area = DialogAddItem(table2, DITEM_AREA);
   DialogItemSetPadding(di, 2, 2, 2, 2);
   DialogItemSetFill(di, 1, 0);
   DialogItemAreaSetSize(di, 160, 120);

   DialogItemSetCallback(slider, CB_AreaDisplayRedraw, 0, (void *)area);
   DialogItemSetCallback(slider2, CB_AreaDisplayRedraw, 0, (void *)area);

   di = DialogAddItem(table, DITEM_SEPARATOR);
   DialogItemSetPadding(di, 2, 2, 2, 2);
   DialogItemSetFill(di, 1, 0);
   DialogItemSeparatorSetOrientation(di, 0);

   di = DialogAddItem(table, DITEM_CHECKBUTTON);
   DialogItemSetPadding(di, 2, 2, 2, 2);
   DialogItemSetFill(di, 1, 0);
   DialogItemSetText(di, _("Wrap virtual desktops around"));
   DialogItemCheckButtonSetPtr(di, &tmp_area_wraparound);

   di = DialogAddItem(table, DITEM_CHECKBUTTON);
   DialogItemSetPadding(di, 2, 2, 2, 2);
   DialogItemSetFill(di, 1, 0);
   DialogItemSetText(di, _("Enable edge flip"));
   DialogItemCheckButtonSetPtr(di, &tmp_edge_flip);

   di = DialogAddItem(table, DITEM_TEXT);
   DialogItemSetPadding(di, 2, 2, 2, 2);
   DialogItemSetFill(di, 1, 0);
   DialogItemSetText(di, _("Resistance at edge of screen:\n"));

   di = slider = DialogAddItem(table, DITEM_SLIDER);
   DialogItemSliderSetMinLength(di, 10);
   DialogItemSetPadding(di, 2, 2, 2, 2);
   DialogItemSetFill(di, 1, 0);
   DialogItemSliderSetBounds(di, 1, 100);
   DialogItemSliderSetUnits(di, 1);
   DialogItemSliderSetJump(di, 10);
   DialogItemSliderSetValPtr(di, &tmp_edge_resist);

   di = DialogAddItem(table, DITEM_SEPARATOR);
   DialogItemSetPadding(di, 2, 2, 2, 2);
   DialogItemSetFill(di, 1, 0);
   DialogItemSeparatorSetOrientation(di, 0);

   DialogAddButton(d, _("OK"), CB_ConfigureAreas, 1, DIALOG_BUTTON_OK);
   DialogAddButton(d, _("Apply"), CB_ConfigureAreas, 0, DIALOG_BUTTON_APPLY);
   DialogAddButton(d, _("Close"), CB_ConfigureAreas, 1, DIALOG_BUTTON_CLOSE);
   DialogSetExitFunction(d, CB_ConfigureAreas, 2);
   DialogBindKey(d, "Escape", DialogCallbackClose, 0);
   DialogBindKey(d, "Return", CB_ConfigureAreas, 0);
   ShowDialog(d);
   CB_AreaDisplayRedraw(d, 1, area);
}

/*
 * IPC functions
 */

static void
DeskOpGoto(unsigned int desk)
{
   Desk               *dsk;
   Desk               *pd = DesksGetCurrent();

   if (Conf.desks.desks_wraparound)
      desk %= Conf.desks.num;
   if (desk >= Conf.desks.num)
      return;

   dsk = _DeskGet(desk);

   DeskGoto(dsk);

   if (DesksGetCurrent() != pd)
      SoundPlay("SOUND_DESKTOP_SHUT");
}

static void
DeskOpGotoRel(int drel)
{
   DeskOpGoto(DesksGetCurrentNum() + drel);
}

static void
DeskOpDrag(int desk)
{
   DeskDragStart(desk);
}

static void
DesksIpcDesk(const char *params, Client * c __UNUSED__)
{
   const char         *p;
   char                cmd[128], prm[128];
   int                 len;
   unsigned int        desk;

   cmd[0] = prm[0] = '\0';
   p = params;
   if (p)
     {
	len = 0;
	sscanf(p, "%100s %100s %n", cmd, prm, &len);
	p += len;
     }

   desk = DesksGetCurrentNum();

   if (!p || cmd[0] == '?')
     {
	IpcPrintf("Current Desktop: %d/%d\n", desk, Conf.desks.num);
     }
   else if (!strncmp(cmd, "cfg", 3))
     {
	SettingsDesktops();
     }
   else if (!strncmp(cmd, "set", 3))
     {
	sscanf(prm, "%i", &desk);
	ChangeNumberOfDesktops(desk);
     }
   else if (!strncmp(cmd, "list", 2))
     {
	Desk               *dsk;

	for (desk = 0; desk < Conf.desks.num; desk++)
	  {
	     dsk = _DeskGet(desk);
	     IpcPrintf("Desk %d: x,y=%d,%d w,h=%d,%d viewable=%d order=%d\n",
		       desk, EoGetX(dsk), EoGetY(dsk), EoGetW(dsk), EoGetH(dsk),
		       dsk->viewable, desks.order[desk]);
	  }
     }
   else if (!strncmp(cmd, "goto", 2))
     {
	sscanf(prm, "%i", &desk);
	DeskOpGoto(desk);
     }
   else if (!strncmp(cmd, "next", 2))
     {
	DeskOpGotoRel(1);
     }
   else if (!strncmp(cmd, "prev", 2))
     {
	DeskOpGotoRel(-1);
     }
   else if (!strncmp(cmd, "this", 2))
     {
	DeskOpGotoRel(0);
     }
   else if (!strncmp(cmd, "raise", 2))
     {
	sscanf(prm, "%i", &desk);
	SoundPlay("SOUND_DESKTOP_RAISE");
	DeskRaise(desk);
     }
   else if (!strncmp(cmd, "lower", 2))
     {
	sscanf(prm, "%i", &desk);
	SoundPlay("SOUND_DESKTOP_LOWER");
	DeskLower(desk);
     }
   else if (!strcmp(cmd, "drag"))
     {
	if (prm[0])
	   desk = atoi(prm);
	DeskOpDrag(desk);
     }
   else if (!strcmp(cmd, "dragbar"))
     {
	if (!strncmp(prm, "dir", 3))
	   DeskDragdirSet(p);
	else if (!strncmp(prm, "ord", 3))
	   DeskDragbarOrderSet(p);
     }
}

static void
DesksIpcArea(const char *params, Client * c __UNUSED__)
{
   const char         *p;
   char                cmd[128], prm[128];
   int                 len;
   int                 ax, ay, dx, dy;

   cmd[0] = prm[0] = '\0';
   p = params;
   if (p)
     {
	len = 0;
	sscanf(p, "%100s %100s %n", cmd, prm, &len);
	p += len;
     }

   DeskGetCurrentArea(&ax, &ay);

   if (!p || cmd[0] == '?')
     {
	IpcPrintf("Current Area: %d %d\n", ax, ay);
     }
   else if (!strncmp(cmd, "cfg", 3))
     {
	SettingsArea();
     }
   else if (!strncmp(cmd, "set", 3))
     {
	sscanf(params, "%*s %i %i", &ax, &ay);
	SetNewAreaSize(ax, ay);
     }
   else if (!strncmp(cmd, "goto", 2))
     {
	sscanf(params, "%*s %i %i", &ax, &ay);
	SetCurrentArea(ax, ay);
     }
   else if (!strncmp(cmd, "move", 2))
     {
	dx = dy = 0;
	sscanf(params, "%*s %i %i", &dx, &dy);
	MoveCurrentAreaBy(dx, dy);
     }
   else if (!strncmp(cmd, "lgoto", 2))
     {
	sscanf(params, "%*s %i", &ax);
	SetCurrentLinearArea(ax);
     }
   else if (!strncmp(cmd, "lmove", 2))
     {
	dx = 0;
	sscanf(params, "%*s %i", &dx);
	MoveCurrentLinearAreaBy(dx);
     }
}

IpcItem             DesksIpcArray[] = {
   {
    DesksIpcDesk,
    "desk", NULL,
    "Desktop functions",
    "  desk ?               Desktop info\n"
    "  desk cfg             Configure desktops\n"
    "  desk drag            Start deskdrag\n"
    "  desk set <nd>        Set number of desktops\n"
    "  desk goto <d>        Goto specified desktop\n"
    "  desk next            Goto next desktop\n"
    "  desk prev            Goto previous desktop\n"
    "  desk this            Goto this desktop\n"
    "  desk lower <d>       Lower desktop\n"
    "  desk raise <d>       Raise desktop\n"
    "  desk dragbar pos     Set dragbar position\n"
    "  desk dragbar order   Set dragbar button order\n"}
   ,
   {
    DesksIpcArea,
    "area", NULL,
    "Area functions",
    "  area ?               Area info\n"
    "  area cfg             Configure areas\n"
    "  area set <nx> <ny>   Set area size\n"
    "  area goto <ax> <ay>  Goto specified area\n"
    "  area move <dx> <dy>  Move relative to current area\n"
    "  area lgoto <al>      Goto specified linear area\n"
    "  area lmove <dl>      Move relative to current linear area\n"}
   ,
};
#define N_IPC_FUNCS (sizeof(DesksIpcArray)/sizeof(IpcItem))

static void
DesksCfgFuncCount(void *item __UNUSED__, const char *value)
{
   ChangeNumberOfDesktops(atoi(value));
}

static void
DesksCfgFuncDragdir(void *item __UNUSED__, const char *value)
{
   DeskDragdirSet(value);
}

static void
DesksCfgFuncDragdbarOrder(void *item __UNUSED__, const char *value)
{
   DeskDragbarOrderSet(value);
}

static void
AreasCfgFuncSizeX(void *item __UNUSED__, const char *value)
{
   int                 ax, ay;

   GetAreaSize(&ax, &ay);
   SetNewAreaSize(atoi(value), ay);
}

static void
AreasCfgFuncSizeY(void *item __UNUSED__, const char *value)
{
   int                 ax, ay;

   GetAreaSize(&ax, &ay);
   SetNewAreaSize(ax, atoi(value));
}

static const CfgItem DesksCfgItems[] = {
   CFG_FUNC_INT(Conf.desks, num, 2, DesksCfgFuncCount),
   CFG_FUNC_INT(Conf.desks, dragdir, 2, DesksCfgFuncDragdir),
   CFG_ITEM_INT(Conf.desks, dragbar_width, 16),
   CFG_ITEM_INT(Conf.desks, dragbar_length, 0),
   CFG_FUNC_INT(Conf.desks, dragbar_ordering, 1, DesksCfgFuncDragdbarOrder),
   CFG_ITEM_BOOL(Conf.desks, desks_wraparound, 0),
   CFG_ITEM_BOOL(Conf.desks, slidein, 1),
   CFG_ITEM_INT(Conf.desks, slidespeed, 6000),

   CFG_FUNC_INT(Conf.desks, areas_nx, 2, AreasCfgFuncSizeX),
   CFG_FUNC_INT(Conf.desks, areas_ny, 1, AreasCfgFuncSizeY),
   CFG_ITEM_BOOL(Conf.desks, areas_wraparound, 0),
};
#define N_CFG_ITEMS (sizeof(DesksCfgItems)/sizeof(CfgItem))

/*
 * Module descriptor
 */
EModule             ModDesktops = {
   "desktops", "desk",
   DesksSighan,
   {N_IPC_FUNCS, DesksIpcArray},
   {N_CFG_ITEMS, DesksCfgItems}
};
