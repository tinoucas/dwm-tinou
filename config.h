/* See LICENSE file for copyright and license details. */

/* appearance */
//static const char font[]            = "-*-helvetica-bold-r-*-*-12-*-*-*-*-*-*-1";
//static const char font[]            = "-*-freemono-medium-r-normal-*-*-*-*-*-*-*-*-1";
//static const char font[]            = "-*-clean-medium-r-normal-*-12-*-*-*-*-*-iso8859-1";
static const char font[]            = "-*-fixed-medium-r-*-*-13-*-*-*-*-*-*-1";
static const char ttffont[]         = "XinGothic-9";
//static const char font[]            = "-*-proggyclean-medium-*-*-*-*-*-*-*-*-*-*-1";
static const char histfile[]        = "/home/tinou/.surf/history.dmenu";
static const char normbordercolor[] = "#444444";
static const char normbgcolor[]     = "#222222";
static const char normfgcolor[]     = "#bbbbbb";
static const char selbordercolor[]  = "#ffffff";
static const char selbgcolor[]      = "#005577";
static const char selfgcolor[]      = "#eeeeee";
static const unsigned int borderpx  = 2;        /* border pixel of windows */
static const unsigned int snap      = 32;       /* snap pixel */
static const unsigned int systraypinning = 0;   /* 0: sloppy systray follows selected monitor, >0: pin systray to monitor X */
static const unsigned int systrayspacing = 2;   /* systray spacing */
static const Bool systraypinningfailfirst = True;   /* True: if pinning fails, display systray on the first monitor, False: display systray on the last monitor*/
static const Bool showsystray       = True;     /* False means no systray */
static const Bool showbar           = True;     /* False means no bar */
static const Bool topbar            = True;    /* False means bottom bar */
static const int windowgap			= 14; /* gap between windows */

/* tagging */
static const char *tags[] = { "1", "2", "3", "4", "5", "6", "7", "8", "9" };

#define Button6 (Button1 + 5)
#define Button7 (Button1 + 6)
#define Button8 (Button1 + 7)
#define Button9 (Button1 + 8)
#define Button10 (Button1 + 9)
#define Button11 (Button1 + 10)
#define Button12 (Button1 + 11)

#include "mousemap.c"

#define TRANS 0.75
#define OPAQU 1.0

#include "flextile.h"
#include "push.c"
#include "ctrlmap.c"
#include "fibonacci.c"
#include <X11/XF86keysym.h>

static const Layout layouts[] = {
	/* symbol     arrange function */
	{ "[]=",      tile },
	{ "[]@",      spiral },
	{ "[]G",      dwindle },
	{ "><>",      NULL },    /* no layout function means floating behavior */
	{ "[M]",      monocle },
};

enum layout {
	TILE = 0,
	SPIRAL,
	DWINDLE,
	FLOAT,
	MONOCLE,
};

static const Rule rules[] = {
	/* xprop(1):
	 * 
	 *	WM_CLASS(STRING) = instance, class
	 *	WM_NAME(STRING) = title
	 */
	/* class				instance		title			tags mask	isfloating	transp	nofocus noborder rh	monitor custommouse preflayout */
	{	NULL,				NULL,			NULL,			0,			False,		OPAQU,	False,	False, True,-1, NULL, NULL },
	{	"veromix",			NULL,			NULL,			0,			True,		TRANS,	False,	False, True,-1, NULL, NULL },
	{	"URxvt",			NULL,			NULL,			1 << 0,			False,		TRANS,	False,	False, True,-1, NULL, NULL },
	{	"URxvt",			"screen",		NULL,			1 << 0,		False,		TRANS,	False,	False, True,-1, NULL, NULL },
	{	NULL,				"xterm",		NULL,			0,			False,		TRANS,	False,	False, True,-1, NULL, &layouts[MONOCLE] },
	{	"Gimp",				NULL,			NULL,			1 << 4,		False,		OPAQU,	False,	False, False,-1, NULL, NULL },
	{	"Firefox",			NULL,			NULL,			1 << 8,		False,		OPAQU,	False,	False, True, -1, chrome, NULL },
	{	"vivaldi-stable",	NULL,			NULL,			1 << 8,		False,		OPAQU,	False,	False, True, -1, chrome, NULL },
	{	NULL,				"Download",		NULL,			1 << 7,		False,		OPAQU,	False,	False, True, -1, NULL, NULL },
	{	NULL,				"Navigator",	NULL,			1 << 8,		False,		OPAQU,	False,	False, True, -1, NULL, NULL },
	{	"Gran Paradiso",	NULL,			NULL,			1 << 8,		False,		OPAQU,	False,	False, True, -1, NULL, NULL },
	{	"Opera",			NULL,			NULL,			1 << 8,		False,		OPAQU,	False,	False, True,-1, NULL, NULL },
	{	"Google-chrome",	"google-chrome",NULL,			1 << 8,		False,		OPAQU,	False,	False, True, -1, chrome, &layouts[MONOCLE] },
	{	"google-chrome",	"crx_knipolnnllmklapflnccelgolnpehhpl",	NULL,	1 << 1,	False,	OPAQU,	False, False,	True,	-1, chrome, &layouts[MONOCLE] },
	{	NULL,				"chrome_app_list",NULL,			0,			True,		OPAQU,	False,	False, True,-1 , chrome, NULL },
	{	"Chromium",			"crx_nckgahadagoaajjgafhacjanaoiihapd",NULL,1 << 1,False,TRANS,	False,	False, True,-1 , chrome, &layouts[MONOCLE] },
	{	"Chromium",			"chromium",		NULL,			1 << 8,		False,		OPAQU,	False,	False, True, -1 , chrome, &layouts[MONOCLE] },
	{	NULL,				NULL,			"aura_root_0",	1 << 7,		False,		OPAQU,	False,	False, False, -1 , NULL, NULL },
	{	NULL,				"Pidgin",		NULL,			1 << 1,		False,		TRANS,	False,	False, True,-1 , NULL, NULL },
	{	NULL,				"sonata",		NULL,			1 << 5,		False,		TRANS,	False,	False, True,-1 , NULL, NULL },
	{	NULL,				"ario",			NULL,			1 << 5,		False,		TRANS,	False,	False, True,-1 , NULL, NULL },
	{	"Gmpc",				NULL,			NULL,			1 << 5,		False,		TRANS,	False,	False, True,-1 , NULL, NULL },
	{	"Shredder",			NULL,			NULL,			1 << 1,		False,		TRANS,	False,	False, True, -1 , NULL, NULL },
	{	NULL,				"screen",		NULL,			1,			False,		TRANS,	False,	False, True,-1 , NULL, NULL },
	{	"feh",				NULL,			NULL,			0,			True,		OPAQU,	False,	False, True,-1 , NULL, NULL },
	{	NULL,				"savebox",		NULL,			0,			True,		TRANS,	False,	False, True,-1 , NULL, NULL },
	{	"Xfe",				NULL,			NULL,			1 << 2,		False,		TRANS,	False,	False, True,-1 , NULL, NULL },
	{	NULL,				"ROX-Filer",	NULL,			1 << 2,		False,		TRANS,	False,	False, True,-1 , NULL, NULL },
	{	NULL,				"spacefm",		NULL,			1 << 2,		False,		TRANS,	False,	False, True,-1 , NULL, NULL },
	{	NULL,				"thunar",		NULL,			1 << 2,		False,		TRANS,	False,	False, True,-1 , NULL, NULL },
	{	"Pantheon-files",	"pantheon-files", NULL,			1 << 2,		False,		TRANS,	False,	False, True,-1 , NULL, NULL },
	{	NULL,				NULL,			"Rename",		0,			True,		TRANS,	False,	False, True,-1 , NULL, NULL },
	{	NULL,				NULL,			"Delete",		0,			True,		TRANS,	False,	False, True,-1 , NULL, NULL },
	{	NULL,				NULL,			"Copy",			0,			True,		TRANS,	False,	False, True,-1 , NULL, NULL },
	{	NULL,				NULL,			"Move",			0,			True,		TRANS,	False,	False, True,-1 , NULL, NULL },
	{	NULL,				NULL,			"Mount",		0,			True,		TRANS,	False,	False, True,-1 , NULL, NULL },
	{	NULL,				NULL,			"Renommer",		0,			True,		TRANS,	False,	False, True,-1 , NULL, NULL },
	{	NULL,				NULL,			"Supprimer",	0,			True,		TRANS,	False,	False, True,-1 , NULL, NULL },
	{	NULL,				NULL,			"Copier",		0,			True,		TRANS,	False,	False, True,-1 , NULL, NULL },
	{	NULL,				NULL,			"Déplacer",		0,			True,		TRANS,	False,	False, True,-1 , NULL, NULL },
	{	NULL,				NULL,			"Monter",		0,			True,		TRANS,	False,	False, True,-1 , NULL, NULL },
	{	"Audacious",		NULL,			NULL,			1 << 5,		False,		TRANS,	False,	False, True,-1 , NULL, NULL },
	{	"MPlayer",			NULL,			NULL,			1 << 6,		True,		OPAQU,	False,	False, True,-1 , mpv, NULL },
	{	"mpv",				NULL,			NULL,			1 << 6,		True,		OPAQU,	False,	False, True,-1 , mpv, NULL },
	{	"Vlc",				NULL,			NULL,			0,			False,		OPAQU,	False,	False, True,-1 , NULL, NULL },
	{	"Gcalctool",		NULL,			NULL,			0,			True,		OPAQU,	False,	False, True,-1 , NULL, NULL },
	{	NULL,				"gqmpeg",		NULL,			1 << 5,		True,		OPAQU,	False,	False, True,-1 , NULL, NULL },
	{	"GQmpeg",			"playlist",		NULL,			1 << 1,		False,		OPAQU,	False,	False, True,-1 , NULL, NULL },
	{	NULL,				"oclock",		NULL,			~0,			True,		0.33,	True,	True, True,-1 , NULL, NULL },
	{	NULL,				"TzClock",		NULL,			~0,			True,		TRANS,	True,	True, True,-1 , NULL, NULL },
	{	"Guimup",			"guimup",		NULL,			1 << 5,		False,		OPAQU,	False,	False, True,-1 , NULL, NULL },
	{	NULL,				"uzbl-core",	NULL,			1 << 8,		False,		OPAQU,	False,	False, True,-1 , NULL, NULL },
	{	NULL,				"gvim",			NULL,			1 << 3,		False,		TRANS,	False,	False, False,-1 , chrome, NULL },
	{	NULL,				"vim",			NULL,			1 << 3,		False,		TRANS,	False,	False, False,-1 , chrome, NULL },
	{	"Sublime_text",		NULL,			NULL,			1 << 3,		False,		TRANS,	False,	False, True,-1 , NULL, NULL },
	{	NULL,			"MixVibes Cross",	NULL,			1 << 4,		False,		OPAQU,	False,	False, True, -1 , NULL, NULL },
	{	NULL,			"Cross Preferences",NULL,			1 << 4,		True,		OPAQU,	False,	False, True, -1 , NULL, NULL },
	{"OpenOfficeorg 3.2",	NULL,			NULL,			1 << 4,		False,		TRANS,	False,	False, True, -1 , NULL, NULL },
	{	"Evince",			NULL,			NULL,			1 << 4,		False,		OPAQU,	False,	False, True, -1 , NULL, NULL },
	{	"FBReader",			NULL,			NULL,			1 << 4,		False,		TRANS,	False,	False, True, -1 , NULL, NULL },
	{	NULL,				"stalonetray",	NULL,			~0,			True,		OPAQU,	True,	True, True, -1 , NULL, NULL },
	{   "Display",			NULL,			NULL,			1 << 0,		True,		OPAQU,	True,	True, True, -1 , NULL, NULL },
	{	"broken",			NULL,			"Renoise",		1 << 7,		False,		OPAQU,	False,	False, True,-1 , NULL, NULL },
	{"jetbrains-android-studio", NULL,		NULL,			1 << 7,		False,		TRANS,	False,	False, False,-1, NULL, NULL },
	{"emulator64-arm",		NULL,			NULL,			1 << 5,		True,		OPAQU,	False,	False, True,-1 , NULL, NULL },
	{	"Deadbeef",			"deadbeef",		NULL,			1 << 5,		False,		TRANS,	False,	False, True,-1 , NULL, NULL },
	{	"TelegramDesktop",	"telegram-desktop",	NULL,		1 << 1,		False,		OPAQU,	False,	False, True,-1 , NULL, NULL },
	{	"Steam",			"Steam",		NULL,			1 << 7,		False,		OPAQU,	False,	False, True,-1 , NULL, NULL },
	{	"Steam.exe",		"Steam.exe",	NULL,			1 << 7,		False,		OPAQU,	False,	True, False,-1 , NULL, NULL },
	{   NULL, "google play music desktop player", NULL,		1 << 7,		False,		TRANS,	False,	False, True,-1 , NULL, NULL },
	//{	"Torchlight2.bin.x86_64", NULL,		NULL,			0,			False,		OPAQU,	False,	False, True,-1, torchlight, NULL },
};

static const int layoutaxis[] = {
	1,    /* layout axis: 1 = x, 2 = y; negative values mirror the layout, setting the master area to the right / bottom instead of left / top */
	2,    /* master axis: 1 = x (from left to right), 2 = y (from top to bottom), 3 = z (monocle) */
	2,    /* stack axis:  1 = x (from left to right), 2 = y (from top to bottom), 3 = z (monocle) */
};
/* layout(s) */
static const float mfact      = 0.55; /* factor of master area size [0.05..0.95] */
// static const Bool resizehints = True; /* True means respect size hints in tiled resizals */

#include "tinou.c"

static int initlayout = 0;
static double barOpacity = 0.65;

/* key definitions */
#define MODKEY Mod4Mask
#define TAGKEYS(KEY,TAG) \
	{ MODKEY,                       KEY,      view,           {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask,           KEY,      toggleview,     {.ui = 1 << TAG} }, \
	{ MODKEY|ShiftMask,             KEY,      tag,            {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask|ShiftMask, KEY,      toggletag,      {.ui = 1 << TAG} },

/* helper for spawning shell commands in the pre dwm-5.0 fashion */
#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }

#define WORKSPACE(KEY,TAG) \
	{ MODKEY,   KEY,      view,     {.ui = TAG} },

/* commands */
static const char *dclipcmd[] = { "dclip", "paste", "-fn", font, "-nb", normbgcolor, "-nf", normfgcolor, "-sb", selbgcolor , "-sf", selfgcolor, NULL };
static const char *dmenucmd[] = { "./bin/dmenu_run", "-fn", ttffont, "-nb", normbgcolor, "-nf", normfgcolor, "-sb", selbgcolor, "-sf", selfgcolor, NULL };
//static const char *dmenucmd[] = { "dmenu_run", "-fn", font, "-nb", normbgcolor, "-nf", normfgcolor, "-sb", selbgcolor, "-sf", selfgcolor, NULL };
static const char *termcmd[]  = { "urxvt", NULL };
static const char *screencmd[]  = { "urxvt", "-e", "screen", "-xRR", NULL };

static Key keys[] = {
	/* modifier                     key        function        argument */
	{ ControlMask,                  XK_dollar,   sendbracketright,{0} },
	{ ControlMask,                  XK_asterisk, sendbackslash,   {0} },
	{ ControlMask,           XK_dead_circumflex, sendbracketleft, {0} },
	{ MODKEY,                       XK_l,      spawn,             SHCMD("xtrlock") },
	{ MODKEY,                       XK_o,      jumpviewstackout,  {0} },
	{ MODKEY,                       XK_i,      jumpviewstackin,   {0} },
	{ MODKEY,                       XK_Menu,   focuslast,      {0} },
	{ MODKEY,                       XK_p,      spawn,          {.v = dmenucmd } },
	/*{ MODKEY,                       XK_r,      spawn,          {.v = dmenucmd } },*/
	{ MODKEY|ShiftMask,             XK_Return, spawn,          {.v = termcmd } },
	{ MODKEY|ShiftMask,             XK_x,      spawn,          {.v = screencmd } },
	{ MODKEY,                       XK_b,      togglebar,      {0} },
	{ MODKEY,                       XK_j,      focusstack,     {.i = +1 } },
	{ MODKEY,                       XK_k,      focusstack,     {.i = -1 } },
	{ MODKEY,                       XK_h,      setmfact,       {.f = -0.05} },
	{ MODKEY,                       XK_l,      setmfact,       {.f = +0.05} },
	{ MODKEY,                       XK_Return, zoom,           {0} },
	{ MODKEY,                       XK_Tab,    view,           {0} },
	{ MODKEY|ShiftMask,             XK_c,      killclient,     {0} },
	{ MODKEY,                       XK_t,      setlayout,      {.v = &layouts[TILE]} },
	{ MODKEY,                       XK_f,      setlayout,      {.v = &layouts[FLOAT]} },
	{ MODKEY,                       XK_m,      setlayout,      {.v = &layouts[MONOCLE]} },
	{ MODKEY,                       XK_s,      setlayout,      {.v = &layouts[SPIRAL]} },
	{ MODKEY,                       XK_d,      setlayout,      {.v = &layouts[DWINDLE]} },
	{ MODKEY,                       XK_space,  toggleviews,    {.ui = (1 << 5 | 1 << 6)} },
	{ MODKEY|ShiftMask,             XK_space,  togglefloating, {0} },
	{ MODKEY|ControlMask|ShiftMask, XK_space,  allnonfloat,       {0} },
	{ MODKEY,                       XK_0,      view,           {.ui = ~0 } },
	{ MODKEY|ShiftMask,             XK_0,      tag,            {.ui = ~0 } },
	{ MODKEY,                       XK_comma,  focusmon,       {.i = -1 } },
	{ MODKEY,                       XK_period, focusmon,       {.i = +1 } },
	{ MODKEY|ShiftMask,             XK_comma,  tagmon,         {.i = -1 } },
	{ MODKEY|ShiftMask,             XK_period, tagmon,         {.i = +1 } },
	{ MODKEY|ControlMask,           XK_j,      pushdown,       {0} },
	{ MODKEY|ControlMask,           XK_k,      pushup,         {0} },
	{ MODKEY|ControlMask,           XK_c,      spawn,          SHCMD("exec dclip copy") },
	{ MODKEY|ControlMask,           XK_v,      spawn,          {.v = dclipcmd } },
	TAGKEYS(                        XK_1,                      0)
	TAGKEYS(                        XK_2,                      1)
	TAGKEYS(                        XK_3,                      2)
	TAGKEYS(                        XK_4,                      3)
	TAGKEYS(                        XK_5,                      4)
	TAGKEYS(                        XK_6,                      5)
	TAGKEYS(                        XK_7,                      6)
	TAGKEYS(                        XK_8,                      7)
	TAGKEYS(                        XK_9,                      8)
	TAGKEYS(                        XK_ampersand,              0)
	TAGKEYS(                        XK_eacute,                 1)
	TAGKEYS(                        XK_quotedbl,               2)
	TAGKEYS(                        XK_apostrophe,             3)
	TAGKEYS(                        XK_parenleft,              4)
	TAGKEYS(                        XK_minus,                  5)
	TAGKEYS(                        XK_egrave,                 6)
	TAGKEYS(                        XK_underscore,             7)
	TAGKEYS(                        XK_ccedilla,               8)
	WORKSPACE(                      XK_a,                      1 << 1 | 1 << 5)
	WORKSPACE(                      XK_w,                      1 << 0 | 1 << 3)
	{ MODKEY|ShiftMask,             XK_q,      quit,           {0} },
	{ MODKEY|ControlMask,           XK_t,      rotatelayoutaxis, {.i = 0} },    /* 0 = layout axis */
	{ MODKEY|ControlMask,           XK_m,      rotatelayoutaxis, {.i = 1} },    /* 1 = master axis */
	{ MODKEY|ControlMask,           XK_s,      rotatelayoutaxis, {.i = 2} },    /* 2 = stack axis */
	{ MODKEY|ControlMask,           XK_Return, mirrorlayout,     {0} },
	{ MODKEY|ControlMask,           XK_l,      shiftmastersplit, {.i = -1} },   /* reduce the number of tiled clients in the master area */
	{ MODKEY|ControlMask,           XK_h,      shiftmastersplit, {.i = +1} },   /* increase the number of tiled clients in the master area */
	{ 0,                            XF86XK_AudioRaiseVolume, spawn, SHCMD("/home/tinou/hacks/scripts/Volume.sh up") },
	{ 0,                            XF86XK_AudioLowerVolume, spawn, SHCMD("/home/tinou/hacks/scripts/Volume.sh down") },
	{ 0,                            XF86XK_AudioMute,        spawn, SHCMD("/home/tinou/hacks/scripts/Volume.sh mute") },
};

/* button definitions */
/* click can be ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
static Button buttons[] = {
	/* click                event mask      button          function        argument */
	//{ ClkClientWin,         0,              Button10,       jumpviewstackin,{0} },
	//{ ClkClientWin,         0,              Button11,       jumpviewstackout,{0} },
	{ ClkWinTitle,          0,              Button10,       jumpviewstackin,{0} },
	{ ClkWinTitle,          0,              Button11,       jumpviewstackout,{0} },
	{ ClkTagBar,            0,              Button4,        jumpviewstackin,{0} },
	{ ClkTagBar,            0,              Button5,        jumpviewstackout,{0} },
	{ ClkLtSymbol,          0,              Button1,        setlayout,      {0} },
	{ ClkLtSymbol,          0,              Button3,        setlayout,      {.v = &layouts[2]} },
	{ ClkLtSymbol,          0,              Button4,        viewscroll,     {.f = -0.05 } },
	{ ClkLtSymbol,          0,              Button5,        viewscroll,     {.f = +0.05 } },
	{ ClkWinTitle,          0,              Button2,        killclient,     {0} },
	{ ClkWinTitle,          0,              Button3,        zoom,           {0} },
	{ ClkWinTitle,          0,              Button4,        ttbarclick,     {.f = -0.05 } },
	{ ClkWinTitle,          0,              Button5,        ttbarclick,     {.f = +0.05 } },
	{ ClkWinTitle,          0,              Button6,        shiftmastersplit,  {.i = +1} },
	{ ClkWinTitle,          0,              Button7,        shiftmastersplit,  {.i = -1} },
	{ ClkWinTitle,          0,              Button8,        rotateclients,  {.i = +1} },
	{ ClkWinTitle,          0,              Button9,        rotateclients,  {.i = -1} },
	{ ClkWinTitle,          MODKEY,         Button4,        opacitychange,  {.f = -0.05 } },
	{ ClkWinTitle,          MODKEY,         Button5,        opacitychange,  {.f = +0.05 } },
	{ ClkStatusText,        MODKEY,         Button2,        spawn,          {.v = termcmd } },
	{ ClkStatusText,        0,              Button3,        spawn,          SHCMD("/home/tinou/bin/toggle /var/lock/tinou/isnet && pkill dwm_sleep") },
	{ ClkStatusText,        0,              Button1,        toggleviews,    {.ui = (1 << 5 | 1 << 6)} },
	{ ClkStatusText,        0,              Button2,        spawn,          SHCMD("/home/tinou/hacks/scripts/Volume.sh mute") },
	{ ClkStatusText,        0,              Button3,        view,           {0} },
	{ ClkStatusText,        0,              Button4,        spawn,          SHCMD("/home/tinou/hacks/scripts/Volume.sh up") },
	{ ClkStatusText,        0,              Button5,        spawn,          SHCMD("/home/tinou/hacks/scripts/Volume.sh down") },
	{ ClkStatusText,        0,              Button8,        togglefloating, {0} },
	{ ClkStatusText,        0,              Button9,        togglefloating, {0} },
	{ ClkClientWin,         MODKEY,         Button1,        movemouse,      {0} },
	{ ClkClientWin,         MODKEY,         Button2,        togglefloating, {0} },
	{ ClkClientWin,         MODKEY,         Button3,        resizemouse,    {0} },
	{ ClkTagBar,            0,              Button1,        view,           {0} },
	{ ClkTagBar,            0,              Button3,        toggleview,     {0} },
	{ ClkTagBar,            0,              Button8,        tag,            {0} },
	{ ClkTagBar,            0,              Button9,        toggletag,      {0} },
	{ ClkTagBar,            MODKEY,         Button1,        tag,            {0} },
	{ ClkTagBar,            MODKEY,         Button3,        toggletag,      {0} },
};

