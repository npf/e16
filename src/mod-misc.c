/*
 * Copyright (C) 2003-2004 Kim Woelders
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

/***********************************************************************
 * *** should all go elsewhere ***
 **********************************************************************/

/* Use static module list for now */
extern EModule      ModAclass;
extern EModule      ModBackgrounds;
extern EModule      ModButtons;

#if USE_COMPOSITE
extern EModule      ModCompMgr;
#endif
extern EModule      ModCursors;
extern EModule      ModDesktops;
extern EModule      ModEffects;
extern EModule      ModEwins;
extern EModule      ModFocus;
extern EModule      ModGroups;
extern EModule      ModImageclass;
extern EModule      ModIconboxes;
extern EModule      ModMenus;
extern EModule      ModMisc;
extern EModule      ModPagers;
extern EModule      ModSlideouts;
extern EModule      ModSound;
extern EModule      ModTextclass;
extern EModule      ModTheme;
extern EModule      ModTooltips;
extern EModule      ModTransparency;
extern EModule      ModWarplist;

const EModule      *p_modules[] = {
   &ModDesktops,
   &ModAclass,
   &ModBackgrounds,
   &ModButtons,
#if USE_COMPOSITE
   &ModCompMgr,
#endif
   &ModCursors,
   &ModEwins,
   &ModEffects,
   &ModFocus,
   &ModGroups,
   &ModIconboxes,
   &ModImageclass,
   &ModMenus,
   &ModMisc,
   &ModPagers,
   &ModSlideouts,
   &ModSound,
   &ModTextclass,
   &ModTheme,
   &ModTooltips,
   &ModTransparency,
   &ModWarplist,
};
int                 n_modules = sizeof(p_modules) / sizeof(EModule *);

static void
runDocBrowser(void)
{
   char                file[FILEPATH_LEN_MAX];

   Esnprintf(file, sizeof(file), "%s/edox", EDirBin());
   if (!canexec(file))
      return;
   Esnprintf(file, sizeof(file), "%s/E-docs", EDirRoot());
   if (!canread(file))
      return;

   if (fork())
      EDBUG_RETURN_;

   Esnprintf(file, sizeof(file), "exec %s/edox %s/E-docs",
	     EDirBin(), EDirRoot());

   execl(usershell(getuid()), usershell(getuid()), "-c", (char *)file, NULL);

   exit(0);
}

static void
SetupUserInitialization(void)
{

   char                file[FILEPATH_LEN_MAX];

   if (!Conf.startup.firsttime)
      return;
   if (!Mode.wm.master)
      return;

   if (fork() == 0)
     {
	Esnprintf(file, sizeof(file), "exec %s/scripts/e_gen_menu", EDirRoot());
	execl(usershell(getuid()), usershell(getuid()), "-c", (char *)file,
	      NULL);
	exit(0);
     }

   /* Run edox */
   runDocBrowser();

   autosave();
}

static void
MiscSighan(int sig, void *prm __UNUSED__)
{
   switch (sig)
     {
     case ESIGNAL_START:
	SetupUserInitialization();
	break;
     }
}

static const CfgItem cfg_items[] = {
   CFG_ITEM_INT(Conf, backgrounds.hiquality, 1),
   CFG_ITEM_INT(Conf, backgrounds.timeout, 240),
   CFG_ITEM_BOOL(Conf, backgrounds.user, 1),

   CFG_ITEM_BOOL(Conf, dialogs.headers, 0),

   CFG_ITEM_BOOL(Conf, dock.enable, 1),
   CFG_ITEM_BOOL(Conf, dock.sticky, 1),
   CFG_ITEM_INT(Conf, dock.dirmode, DOCK_DOWN),
   CFG_ITEM_INT(Conf, dock.startx, 0),
   CFG_ITEM_INT(Conf, dock.starty, 0),

   CFG_ITEM_BOOL(Conf, groups.dflt.iconify, 1),
   CFG_ITEM_BOOL(Conf, groups.dflt.kill, 0),
   CFG_ITEM_BOOL(Conf, groups.dflt.mirror, 1),
   CFG_ITEM_BOOL(Conf, groups.dflt.move, 1),
   CFG_ITEM_BOOL(Conf, groups.dflt.raise, 0),
   CFG_ITEM_BOOL(Conf, groups.dflt.set_border, 1),
   CFG_ITEM_BOOL(Conf, groups.dflt.stick, 1),
   CFG_ITEM_BOOL(Conf, groups.dflt.shade, 1),
   CFG_ITEM_BOOL(Conf, groups.swapmove, 1),

   CFG_ITEM_BOOL(Conf, hints.set_xroot_info_on_root_window, 0),

   CFG_ITEM_INT(Conf, movres.mode_move, 0),
   CFG_ITEM_INT(Conf, movres.mode_resize, 2),
   CFG_ITEM_INT(Conf, movres.mode_info, 1),
   CFG_ITEM_INT(Conf, movres.opacity, 150),

   CFG_ITEM_BOOL(Conf, place.manual, 0),
   CFG_ITEM_BOOL(Conf, place.manual_mouse_pointer, 0),
   CFG_ITEM_BOOL(Conf, place.ignore_struts, 0),
   CFG_ITEM_BOOL(Conf, place.raise_fullscreen, 0),

   CFG_ITEM_BOOL(Conf, snap.enable, 1),
   CFG_ITEM_INT(Conf, snap.edge_snap_dist, 8),
   CFG_ITEM_INT(Conf, snap.screen_snap_dist, 32),

   CFG_ITEM_BOOL(Conf, startup.firsttime, 1),
   CFG_ITEM_BOOL(Conf, startup.animate, 1),

   CFG_ITEM_INT(Conf, deskmode, MODE_NONE),
   CFG_ITEM_INT(Conf, slidemode, 0),
   CFG_ITEM_BOOL(Conf, cleanupslide, 1),
   CFG_ITEM_BOOL(Conf, mapslide, 0),
   CFG_ITEM_INT(Conf, slidespeedmap, 6000),
   CFG_ITEM_INT(Conf, slidespeedcleanup, 8000),
   CFG_ITEM_BOOL(Conf, animate_shading, 1),
   CFG_ITEM_INT(Conf, shadespeed, 8000),
   CFG_ITEM_INT(Conf, button_move_resistance, 5),
   CFG_ITEM_BOOL(Conf, autosave, 1),
   CFG_ITEM_BOOL(Conf, memory_paranoia, 1),
   CFG_ITEM_BOOL(Conf, save_under, 0),
   CFG_ITEM_INT(Conf, edge_flip_resistance, 25),
};
#define N_CFG_ITEMS ((int)(sizeof(cfg_items)/sizeof(CfgItem)))

static void
MiscIpcExec(const char *params, Client * c __UNUSED__)
{
   execApplication(params);
}

static void
MiscIpcConfig(const char *params, Client * c __UNUSED__)
{
   const char         *p;
   char                cmd[128], prm[128];
   int                 len;

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
	/* Show info */
     }
   else if (!strncmp(cmd, "cfg", 3))
     {
	if (!strncmp(prm, "autoraise", 2))
	   SettingsAutoRaise();
	else if (!strncmp(prm, "fx", 2))
	   SettingsSpecialFX();
	else if (!strncmp(prm, "misc", 2))
	   SettingsMiscellaneous();
	else if (!strncmp(prm, "moveresize", 2))
	   SettingsMoveResize();
	else if (!strncmp(prm, "placement", 2))
	   SettingsPlacement();
	else if (!strncmp(prm, "remember", 2))
	   SettingsRemember();
     }
   else if (!strncmp(cmd, "arrange", 3))
     {
	ArrangeEwins(prm);
     }
}

IpcItem             MiscIpcArray[] = {
   {
    MiscIpcExec,
    "exec", NULL,
    "Execute program",
    "  exec <command>           Execute command\n"}
   ,
   {
    MiscIpcConfig,
    "misc", NULL,
    "Miscellaneous functions",
    "  misc cfg <thing>         Configure thing (focus/fx/moveresize/placement/remember)\n"}
};
#define N_IPC_FUNCS (sizeof(MiscIpcArray)/sizeof(IpcItem))

/* Stuff not elsewhere */
EModule             ModMisc = {
   "misc", NULL,
   MiscSighan,
   {N_IPC_FUNCS, MiscIpcArray}
   ,
   {N_CFG_ITEMS, cfg_items}
};
