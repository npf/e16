
#include "E.h"
#include <X11/keysym.h>

void
MapUnmap(int start)
{
   /* this function will map and unmap all the windows based on the progress
    * through the startup sequence
    */

   static Window      *wlist = NULL;
   Window              par;
   Window              rt;
   XWindowAttributes   attr;
   static unsigned int num = 0;
   int                 i;

   EDBUG(6, "MapUnmap");
   switch (start)
     {
     case 0:
	XQueryTree(disp, root.win, &rt, &par, &wlist, &num);
	if (wlist)
	  {
	     for (i = 0; i < (int)num; i++)
	       {
		  if ((init_win_ext) && (init_win_ext == wlist[i]))
		    {
		       wlist[i] = 0;
		    }
		  else
		    {
		       XGetWindowAttributes(disp, wlist[i], &attr);
		       if (attr.map_state == IsUnmapped)
			 {
			    wlist[i] = 0;
			 }
		       else
			 {
			    EUnmapWindow(disp, wlist[i]);
			 }
		    }
	       }
	  }
	break;
     case 1:
	if (wlist)
	  {
	     for (i = 0; i < (int)num; i++)
	       {
		  if (wlist[i])
		    {
		       if (XGetWindowAttributes(disp, wlist[i], &attr))
			 {
			    if (attr.override_redirect)
			      {
				 if (init_win1)
				   {
				      XRaiseWindow(disp, init_win1);
				      XRaiseWindow(disp, init_win2);
				   }
				 if (init_win_ext)
				    XRaiseWindow(disp, init_win_ext);
				 ShowEdgeWindows();
				 RaiseProgressbars();
				 EMapWindow(disp, wlist[i]);
			      }
			    else
			      {
				 AddToFamily(wlist[i]);
			      }
			 }
		    }
	       }
	     XFree(wlist);
	  }
	break;
     default:
	break;
     }
   EDBUG_RETURN_;
}

void
SetupSignals()
{

   /* This function will set up all the signal handlers for E */

   struct sigaction    sa;

   EDBUG(6, "SetupSignals");

   sa.sa_handler = HandleSigHup;
   sa.sa_flags = 0;
   sigemptyset(&sa.sa_mask);
   sigaction(SIGHUP, &sa, (struct sigaction *)0);

   sa.sa_handler = HandleSigInt;
   sa.sa_flags = 0;
   sigemptyset(&sa.sa_mask);
   sigaction(SIGINT, &sa, (struct sigaction *)0);

   sa.sa_handler = HandleSigQuit;
   sa.sa_flags = 0;
   sigemptyset(&sa.sa_mask);
   sigaction(SIGQUIT, &sa, (struct sigaction *)0);

   sa.sa_handler = HandleSigIll;
   sa.sa_flags = 0;
   sigemptyset(&sa.sa_mask);
   sigaction(SIGILL, &sa, (struct sigaction *)0);

   sa.sa_handler = HandleSigAbrt;
   sa.sa_flags = 0;
   sigemptyset(&sa.sa_mask);
   sigaction(SIGABRT, &sa, (struct sigaction *)0);

   sa.sa_handler = HandleSigFpe;
   sa.sa_flags = 0;
   sigemptyset(&sa.sa_mask);
   sigaction(SIGFPE, &sa, (struct sigaction *)0);

   sa.sa_handler = HandleSigSegv;
   sa.sa_flags = 0;
   sigemptyset(&sa.sa_mask);
   sigaction(SIGSEGV, &sa, (struct sigaction *)0);

   sa.sa_handler = HandleSigPipe;
   sa.sa_flags = 0;
   sigemptyset(&sa.sa_mask);
   sigaction(SIGPIPE, &sa, (struct sigaction *)0);

   sa.sa_handler = HandleSigAlrm;
   sa.sa_flags = 0;
   sigemptyset(&sa.sa_mask);
   sigaction(SIGALRM, &sa, (struct sigaction *)0);

   sa.sa_handler = HandleSigTerm;
   sa.sa_flags = 0;
   sigemptyset(&sa.sa_mask);
   sigaction(SIGTERM, &sa, (struct sigaction *)0);

   sa.sa_handler = HandleSigUsr1;
   sa.sa_flags = 0;
   sigemptyset(&sa.sa_mask);
   sigaction(SIGUSR1, &sa, (struct sigaction *)0);

   sa.sa_handler = HandleSigUsr2;
   sa.sa_flags = 0;
   sigemptyset(&sa.sa_mask);
   sigaction(SIGUSR2, &sa, (struct sigaction *)0);

   sa.sa_handler = HandleSigChild;
   sa.sa_flags = SA_RESTART;
   sigemptyset(&sa.sa_mask);
   sigaction(SIGCHLD, &sa, (struct sigaction *)0);

   sa.sa_handler = HandleSigBus;
   sa.sa_flags = 0;
   sigemptyset(&sa.sa_mask);
   sigaction(SIGBUS, &sa, (struct sigaction *)0);

   EDBUG_RETURN_;
}

void
SetupX()
{

   /* This function sets up all of our connections to X */

   int                 shape_event_base, shape_error_base, i;

   EDBUG(6, "SetupX");

   /* In case we are going to fork, set up the master pid */
   master_pid = getpid();

   /* Open a connection to the diplay nominated by the DISPLAY variable */
   disp = XOpenDisplay(NULL);
   /* if cannot connect to display */
   if (!disp)
     {
	Alert("Enlightenment cannot connect to the display nominated by\n"
	      "your shell's DISPLAY environment variable. You may set this\n"
	      "variable to indicate which display name Enlightenment is to\n"
	      "connect to. It may be that you do not have an Xserver already\n"
	      "running to serve that Display connection, or that you do not\n"
	      "have permission to connect to that display. Please make sure\n"
	      "all is correct before trying again. Run an Xserver by running\n"
	      "xdm or startx first, or contact your local system\n"
	      "administrator, or Xserver vendor, or read the X, xdm and\n"
	      "startx manual pages before proceeding.\n");
	EExit((void *)1);
     }
   root.scr = DefaultScreen(disp);
   display_screens = ScreenCount(disp);
   master_screen = root.scr;

   /* Start up on multiple heads, if appropriate */
   if (display_screens > 1 && 0 == single_screen_mode)
     {
	int                 i;
	char                subdisplay[255];
	char               *dispstr;

	dispstr = DisplayString(disp);

	strcpy(subdisplay, DisplayString(disp));

	for (i = 0; i < display_screens; i++)
	  {
	     pid_t               pid;

	     if (i != root.scr)
	       {
		  pid = fork();
		  if (pid)
		    {
		       child_count++;
		       e_children = Erealloc(e_children, sizeof(pid_t) * child_count);
		       e_children[child_count - 1] = pid;
		    }
		  else
		    {
		       kill(getpid(), SIGSTOP);
		       /* Find the point to concatenate the screen onto */
		       dispstr = strchr(subdisplay, ':');
		       if (NULL != dispstr)
			 {
			    dispstr = strchr(dispstr, '.');
			    if (NULL != dispstr)
			       *dispstr = '\0';
			 }
		       Esnprintf(subdisplay + strlen(subdisplay), 255, ".%d", i);
		       disp = XOpenDisplay(subdisplay);
		       root.scr = i;
		       /* Terminate the loop as I am the child process... */
		       break;
		    }
	       }
	  }
     }
   /* set up an error handler for then E would normally have fatal X errors */
   XSetErrorHandler((XErrorHandler) EHandleXError);
   /* set up a handler for when the X Connection goes down */
   XSetIOErrorHandler((XIOErrorHandler) HandleXIOError);
   /* Check for the Shape Extension */
   if (!XShapeQueryExtension(disp, &shape_event_base, &shape_error_base))
     {
	ASSIGN_ALERT("X server setup error",
		     "",
		     "",
		     "Quit Enlightenment");
	Alert("FATAL ERROR:\n"
	      "\n"
	      "This Xserver does not support the Shape extension.\n"
	      "This is required for Enlightenment to run.\n"
	      "\n"
	      "Your Xserver probably is too old or mis-configured.\n"
	      "\n"
	      "Exiting.\n");
	RESET_ALERT;
	EExit((void *)1);
     }
   /* check for the XTEST extension */
/*
 * if (XTestQueryExtension(disp, &test_event_base, &test_error_base, &test_v1, &test_v2))
 * {
 * XTestGrabControl(disp, True); 
 * }
 * else
 * Alert("WARNING:\n"
 * "This Xserver does not support the XTest extension.\n"
 * "This is required for Enlightenment to run properly.\n"
 * "Enlightenment will continue to run, but parts may not.\n"
 * "Work correctly.\n"
 * "Please contact your system administrator, or see the manuals\n"
 * "For your XServer to find out how to enable the XTest\n"
 * "Extension\n");
 */
   /* record the event base for shape change events */
   event_base_shape = shape_event_base;
   /* initialise imlib */
   id = Imlib_init(disp);
   if (!id)
     {
	ASSIGN_ALERT("Imlib initialisation error",
		     "",
		     "",
		     "Quit Enlightenment");
	Alert("FATAL ERROR:\n"
	      "\n"
	      "Enlightenment is unable to initialise Imlib.\n"
	      "\n"
	      "This is unusual. Unable to contiune.\n"
	      "Exiting.\n");
	RESET_ALERT;
	EExit((void *)1);
     }
   fd = Fnlib_init(id);
   if (!fd)
     {
	ASSIGN_ALERT("X server setup error",
		     "",
		     "",
		     "Quit Enlightenment");
	Alert("FATAL ERROR:\n"
	      "\n"
	      "Enlightenment is unable to initialise Fnlib.\n"
	      "\n"
	      "This is unusual. Unable to contiune.\n"
	      "Exiting.\n");
	RESET_ALERT;
	EExit((void *)1);
     }
   root.win = id->x.root;
   root.vis = Imlib_get_visual(id);
   root.depth = id->x.depth;
   root.cmap = Imlib_get_colormap(id);
   root.w = DisplayWidth(disp, root.scr);
   root.h = DisplayHeight(disp, root.scr);
   root.focuswin = ECreateFocusWindow(root.win, -100, -100, 5, 5);
   /* warn, if necessary about visual problems */
   if (DefaultVisual(disp, root.scr) != root.vis)
     {
	ImlibInitParams     p;

	p.flags = PARAMS_VISUALID;
	p.visualid = XVisualIDFromVisual(DefaultVisual(disp, root.scr));
	ird = Imlib_init_with_params(disp, &p);
     }
   else
      ird = NULL;
   /* just in case - set them up again */
   /* set up an error handler for then E would normally have fatal X errors */
   XSetErrorHandler((XErrorHandler) EHandleXError);
   /* set up a handler for when the X Connection goes down */
   XSetIOErrorHandler((XIOErrorHandler) HandleXIOError);
   /* slect all the root window events to start managing */
   XSelectInput(disp, root.win,
		ButtonPressMask | ButtonReleaseMask |
		EnterWindowMask | LeaveWindowMask | ButtonMotionMask |
		PropertyChangeMask | SubstructureRedirectMask | KeyPressMask |
		KeyReleaseMask | PointerMotionMask | ResizeRedirectMask |
		SubstructureNotifyMask);
   XSync(disp, False);
   mode.xselect = 0;
   /* Init XKB to pick up release of alt modifier */
   WarpFocusInitEvents();
   {
      Atom                atom_set;
      CARD32              val;

      atom_set = XInternAtom(disp, "_WIN_DESKTOP_BUTTON_PROXY", False);
      bpress_win = ECreateWindow(root.win, -80, -80, 24, 24, 0);
      val = bpress_win;
      XChangeProperty(disp, root.win, atom_set, XA_CARDINAL,
		      32, PropModeReplace,
		      (unsigned char *)&val, 1);
      XChangeProperty(disp, bpress_win, atom_set, XA_CARDINAL,
		      32, PropModeReplace,
		      (unsigned char *)&val, 1);
   }

   XSync(disp, False);

   /* warn, if necessary about X version problems */
   if (ProtocolVersion(disp) != 11)
     {
	ASSIGN_ALERT("X server version error",
		     "Ignore this error",
		     "",
		     "Quit Enlightenment");
	Alert("WARNING:\n"
	      "This is not an X11 Xserver. It infact talks the X%i protocol.\n"
	      "This may mean Enlightenment will either not function, or\n"
	      "function incorrectly. If it is later than X11, then your\n"
	      "server is one the author of Enlightenment neither have\n"
	      "access to, nor have heard of.\n",
	      ProtocolVersion(disp));
	RESET_ALERT;
     }
   /* now we'll set the locale */
   setlocale(LC_ALL, "");
   if (!XSupportsLocale())
      setlocale(LC_ALL, "C");
   XSetLocaleModifiers("");
   setlocale(LC_ALL, NULL);

   /* I dont want any internationalisation of my numeric input & output */
   setlocale(LC_NUMERIC, "C");

   ICCCM_SetIconSizes();
   ICCCM_Focus(NULL);
   MWM_SetInfo();
   initFunctionArray();

   /* damn that bloody numlock stuff - ok I'd rather XFree got fixed to not */
   /* have it as a modifier and everyone have to write specific code to mask */
   /* it out - but well.... */
   /* ok under Xfree Numlock and Scollock are lock modifiers and we need */
   /* to hunt them down to mask them out - EVIL EVIL EVIL hack but needed */
   {
      XModifierKeymap    *mod;
      KeyCode             nl, sl;
      int                 i;
      int                 masks[8] =
      {
	 ShiftMask, LockMask, ControlMask, Mod1Mask, Mod2Mask, Mod3Mask,
	 Mod4Mask, Mod5Mask
      };

      mod = XGetModifierMapping(disp);
      nl = XKeysymToKeycode(disp, XK_Num_Lock);
      sl = XKeysymToKeycode(disp, XK_Scroll_Lock);
      if ((mod) && (mod->max_keypermod > 0))
	{
	   for (i = 0; i < (8 * mod->max_keypermod); i++)
	     {
		if ((nl) && (mod->modifiermap[i] == nl))
		   numlock_mask = masks[i / mod->max_keypermod];
		else if ((sl) && (mod->modifiermap[i] == sl))
		   scrollock_mask = masks[i / mod->max_keypermod];
	     }
	}
      mask_mod_combos[0] = 0;
      mask_mod_combos[1] = LockMask;
      mask_mod_combos[2] = numlock_mask;
      mask_mod_combos[3] = scrollock_mask;
      mask_mod_combos[4] = numlock_mask | scrollock_mask;
      mask_mod_combos[5] = LockMask | numlock_mask;
      mask_mod_combos[6] = LockMask | scrollock_mask;
      mask_mod_combos[7] = LockMask | numlock_mask | scrollock_mask;

      if (mod)
	 XFree(mod);
   }
   /* Now we're going to set a bunch of default settings in E - in case we
    * * don't ever get to load a config file for some odd reason.
    * * Also, we'll take this opportunity to initialize all of our
    * * important state variables.
    */

   mode.next_move_x_plus = 0;
   mode.next_move_y_plus = 0;
   mode.mode = MODE_NONE;
   mode.destroy = 0;
   mode.adestroy = 0;
   mode.deskmode = MODE_NONE;
   mode.place = 0;
   mode.flipp = 0;
   mode.startup = 1;
   mode.xselect = 1;
   mode.ewin = NULL;
   mode.button = NULL;
   mode.have_place_grab = 0;
   mode.noewin = 0;
   mode.px = 0;
   mode.py = 0;
   mode.x = 0;
   mode.y = 0;
   mode.focusmode = FOCUS_SLOPPY;
   mode.focuswin = NULL;
   mode.realfocuswin = NULL;
   mode.mouse_over_win = NULL;
   mode.click_focus_grabbed = 0;
   mode.movemode = 0;
   mode.dockdirmode = DOCK_UP;
   mode.dockstartx = 0;
   mode.dockstarty = 0;
   mode.primaryicondir = ICON_RIGHT;
   mode.resizemode = 1;
   mode.slidemode = 0;
   mode.cleanupslide = 1;
   mode.mapslide = 1;
   mode.slidespeedmap = 6000;
   mode.slidespeedcleanup = 8000;
   mode.shadespeed = 8000;
   mode.animate_shading = 1;
   mode.doingslide = 0;
   mode.server_grabbed = 0;
   mode.desktop_bg_timeout = 240;
   mode.sound = 1;
   mode.button_move_resistance = 5;
   mode.button_move_pending = 0;
   mode.current_cmap = 0;
   mode.autosave = 1;
   mode.slideout = NULL;
   mode.tooltips = 1;
   mode.tiptime = 0.5;
   mode.autoraise = 0;
   mode.autoraisetime = 0.5;
   mode.memory_paranoia = 1;
   mode.save_under = 0;
   mode.cur_menu_mode = 0;
   mode.cur_menu_depth = 0;
   for (i = 0; i < 256; i++)
      mode.cur_menu[i] = NULL;
   mode.menuslide = 0;
   mode.menusonscreen = 1;
   mode.numdesktops = 2;
   mode.transientsfollowleader = 1;
   mode.switchfortransientmap = 1;
   mode.showicons = 1;
   mode.snap = 1;
   mode.edge_snap_dist = 8;
   mode.screen_snap_dist = 32;
   mode.menu_cover_win = 0;
   mode.all_new_windows_get_focus = 0;
   mode.new_transients_get_focus = 0;
   mode.new_transients_get_focus_if_group_focused = 1;
   mode.manual_placement = 0;
   mode.raise_on_next_focus = 1;
   mode.raise_after_next_focus = 1;
   mode.kde_support = 0;
#ifdef WITH_TARTY_WARP
   mode.display_warp = 1;
#else
   mode.display_warp = 0;
#endif /* WITH_TARTY_WARP */
   mode.warp_on_next_focus = 0;
   mode.edge_flip_resistance = 15;
   mode.context_ewin = NULL;
   mode.moveresize_pending_ewin = NULL;
   mode.borderpartpress = 0;
   mode.windowdestroy = 0;
   mode.context_w = 0;
   mode.context_h = 0;
#ifdef AUTOUPGRADE
   mode.autoupgrade = 1;
   mode.activenetwork = 0;
   mode.motddate = 0;
   mode.motd = 1;
#else
   mode.autoupgrade = 0;
   mode.activenetwork = 0;
   mode.motddate = 0;
   mode.motd = 0;
#endif
   mode.show_pagers = 1;
   mode.context_pager = NULL;
   mode.pager_hiq = 1;
   mode.pager_snap = 1;
   mode.user_bg = 1;
   mode.pager_zoom = 1;
   mode.pager_title = 1;
   mode.pager_scanspeed = 10;
   mode.icon_textclass = NULL;
   mode.icon_mode = 2;
   mode.nogroup = 0;
   mode.group_config.iconify = 1;
   mode.group_config.kill = 0;
   mode.group_config.move = 1;
   mode.group_config.raise = 0;
   mode.group_config.set_border = 1;
   mode.group_config.stick = 1;
   mode.group_config.shade = 1;
   mode.group_config.mirror = 1;
   mode.clickalways = 0;

   desks.dragdir = 2;
   desks.dragbar_width = 16;
   desks.dragbar_ordering = 1;
   desks.dragbar_length = 0;
   desks.slidein = 1;
   desks.slidespeed = 6000;
   desks.hiqualitybg = 1;
   SetAreaSize(2, 1);

   for (i = 0; i < ENLIGHTENMENT_CONF_NUM_DESKTOPS; task_menu[i++] = NULL);

   EDBUG_RETURN_;
}

void
SetupDirs()
{
   char                s[1024], ss[1024];

   EDBUG(6, "SetupDirs");
   Esnprintf(s, sizeof(s), "%s", UserEDir());
   if (exists(s))
     {
	if (isfile(s))
	  {
	     Esnprintf(ss, sizeof(ss), "%s.old", UserEDir());
	     mv(s, ss);
	     md(s);
	  }
     }
   else
      md(s);
   Esnprintf(s, sizeof(s), "%s/themes", UserEDir());
   if (!exists(s))
      md(s);
   Esnprintf(s, sizeof(s), "%s/backgrounds", UserEDir());
   if (!exists(s))
      md(s);
   Esnprintf(s, sizeof(s), "%s/cached", UserEDir());
   if (!exists(s))
      md(s);
   Esnprintf(s, sizeof(s), "%s/cached/img", UserEDir());
   if (!exists(s))
      md(s);
   Esnprintf(s, sizeof(s), "%s/cached/cfg", UserEDir());
   if (!exists(s))
      md(s);
   Esnprintf(s, sizeof(s), "%s/cached/bgsel", UserEDir());
   if (!exists(s))
      md(s);
   Esnprintf(s, sizeof(s), "%s/cached/pager", UserEDir());
   if (!exists(s))
      md(s);
   EDBUG_RETURN_;
}

void
SetupEnv()
{
   char                s[1024];

   if (master_pid != getpid())
      Esetenv("DISPLAY", DisplayString(disp), 1);
   Esetenv("EVERSION", ENLIGHTENMENT_VERSION, 1);
   Esetenv("EROOT", ENLIGHTENMENT_ROOT, 1);
   Esnprintf(s, sizeof(s), "%i", getpid());
   Esetenv("EPID", s, 1);
   Esetenv("ETHEME", themepath, 1);

   return;
}

Window
MakeExtInitWin(void)
{
   Display            *d2;
   Window              win;
   XGCValues           gcv;
   GC                  gc;
   Pixmap              pmap;
   Atom                a, aa;
   CARD32              val;
   int                 format_ret, i;
   unsigned long       bytes_after, num_ret;
   Window             *retval;
   XSetWindowAttributes attr;

   ImlibData          *imd;

   a = XInternAtom(disp, "ENLIGHTENMENT_RESTART_SCREEN", False);
   XSync(disp, False);
   if (fork())
     {
	UngrabX();
	for (;;)
	  {
	     retval = NULL;
	     XGetWindowProperty(disp, root.win, a, 0, 0x7fffffff, True,
				XA_CARDINAL, &aa, &format_ret, &num_ret,
				&bytes_after, (unsigned char **)(&retval));
	     XSync(disp, False);
	     if (retval)
		break;
	  }
	win = *retval;
	fflush(stdout);
	XFree(retval);

	return win;
     }
   /* on solairs connection stays up - close */
   XSetErrorHandler((XErrorHandler) NULL);
   XSetIOErrorHandler((XIOErrorHandler) NULL);
   signal(SIGHUP, SIG_DFL);
   signal(SIGINT, SIG_DFL);
   signal(SIGQUIT, SIG_DFL);
   signal(SIGILL, SIG_DFL);
   signal(SIGABRT, SIG_DFL);
   signal(SIGFPE, SIG_IGN);
   signal(SIGSEGV, SIG_IGN);
   signal(SIGPIPE, SIG_IGN);
   signal(SIGALRM, SIG_DFL);
   signal(SIGTERM, SIG_DFL);
   signal(SIGUSR1, SIG_DFL);
   signal(SIGUSR2, SIG_DFL);
   signal(SIGCHLD, SIG_DFL);
   signal(SIGTSTP, SIG_DFL);
   signal(SIGBUS, SIG_IGN);
   d2 = XOpenDisplay(DisplayString(disp));
   close(ConnectionNumber(disp));
   XGrabServer(d2);
   imd = Imlib_init(d2);
   attr.backing_store = NotUseful;
   attr.override_redirect = True;
   attr.colormap = root.cmap;
   attr.border_pixel = 0;
   attr.background_pixel = 0;
   attr.save_under = True;
   win = XCreateWindow(d2, root.win, 0, 0, root.w, root.h, 0, root.depth,
		     InputOutput, root.vis, CWOverrideRedirect | CWSaveUnder |
		    CWBackingStore | CWColormap | CWBackPixel | CWBorderPixel,
		       &attr);
   pmap = ECreatePixmap(d2, win, root.w, root.h, root.depth);
   gcv.subwindow_mode = IncludeInferiors;
   gc = XCreateGC(d2, win, GCSubwindowMode, &gcv);
   XCopyArea(d2, root.win, pmap, gc, 0, 0, root.w, root.h, 0, 0);
   ESetWindowBackgroundPixmap(d2, win, pmap);
   EMapRaised(d2, win);
   EFreePixmap(d2, pmap);
   XFreeGC(d2, gc);
   val = win;
   a = XInternAtom(d2, "ENLIGHTENMENT_RESTART_SCREEN", False);
   XChangeProperty(d2, root.win, a, XA_CARDINAL, 32,
		   PropModeReplace, (unsigned char *)&val, 1);
   XSelectInput(d2, win, StructureNotifyMask);
   XUngrabServer(d2);
   XSync(d2, False);

   if (!imd)
     {
	i = 0;
	for (;;)
	  {
	     Cursor              cs = 0;
	     struct timeval      tv;

	     cs = XCreateFontCursor(d2, i);
	     XDefineCursor(d2, win, cs);
	     XSync(d2, False);
	     tv.tv_sec = 0;
	     tv.tv_usec = 50000;
	     select(0, NULL, NULL, NULL, &tv);
	     XFreeCursor(d2, cs);
	     i += 2;
	     if (i >= XC_num_glyphs)
		i = 0;
	  }
     }
   else
     {
	Window              w2, ww;
	char               *f, s[1024];
	Pixmap              pmap, mask;
	ImlibImage         *im;
	struct timeval      tv;
	int                 dd, x, y;
	unsigned int        mm;
	Cursor              cs = 0;
	XColor              cl;
	GC                  gc;
	XGCValues           gcv;

	w2 = XCreateWindow(d2, win, 0, 0, 32, 32, 0, root.depth,
			   InputOutput, root.vis, CWOverrideRedirect |
			   CWBackingStore | CWColormap | CWBackPixel |
			   CWBorderPixel, &attr);
	pmap = ECreatePixmap(d2, w2, 16, 16, 1);
	gc = XCreateGC(d2, pmap, 0, &gcv);
	XSetForeground(d2, gc, 0);
	XFillRectangle(d2, pmap, gc, 0, 0, 16, 16);
	mask = ECreatePixmap(d2, w2, 16, 16, 1);
	gc = XCreateGC(d2, mask, 0, &gcv);
	XSetForeground(d2, gc, 0);
	XFillRectangle(d2, mask, gc, 0, 0, 16, 16);
	cs = XCreatePixmapCursor(d2, pmap, mask, &cl, &cl, 0, 0);
	XDefineCursor(d2, win, cs);
	XDefineCursor(d2, w2, cs);
	i = 1;
	for (;;)
	  {

	     i++;
	     if (i > 12)
		i = 1;

	     Esnprintf(s, sizeof(s), "pix/wait%i.png", i);

	     f = FindFile(s);
	     im = NULL;
	     if (f)
		im = Imlib_load_image(imd, f);

	     if (f)
		Efree(f);

	     if (im)
	       {
		  Imlib_render(imd, im, im->rgb_width, im->rgb_height);
		  pmap = Imlib_move_image(imd, im);
		  mask = Imlib_move_mask(imd, im);
		  Imlib_destroy_image(imd, im);
		  EShapeCombineMask(d2, w2, ShapeBounding, 0, 0,
				    mask, ShapeSet);
		  ESetWindowBackgroundPixmap(d2, w2, pmap);
		  Imlib_free_pixmap(imd, pmap);
		  XClearWindow(d2, w2);
		  XQueryPointer(d2, win, &ww, &ww, &dd, &dd, &x, &y, &mm);
		  EMoveResizeWindow(d2, w2,
				    x - (im->rgb_width / 2),
				    y - (im->rgb_height / 2),
				    im->rgb_width,
				    im->rgb_height);
		  EMapWindow(d2, w2);
	       }
	     tv.tv_sec = 0;
	     tv.tv_usec = 50000;
	     select(0, NULL, NULL, NULL, &tv);
	     XSync(d2, False);
	  }
     }
/*    {
 * XEvent              ev;
 * 
 * XSync(d2, False);
 * for (;;)
 * {
 * XNextEvent(d2, &ev);
 * }
 * } */
   exit(0);
}
