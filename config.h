/* See LICENSE file for copyright and license details. */

#ifdef CONFIG_HEAD
/* tagging */
static char **tags;
static int numtags;

static const unsigned int maintag = 1 << 0;
static const unsigned int chattag = 1 << 1;
static const unsigned int filestag = 1 << 2;
static const unsigned int texttag = 1 << 3;
static const unsigned int edittag = 1 << 4;
static const unsigned int dltag = 1 << 5;
static const unsigned int trrnttag = 1 << 6;
static const unsigned int musictag = 1 << 7;
static const unsigned int misctag = 1 << 8;
static const unsigned int webtag = 1 << 9;
static unsigned int vtag = 1 << 10;

static const unsigned int anytag = 0;
static const unsigned int alltags = ~0;
#else

/* appearance */
static const char fallbackfont[]	= "DejaVu Sans Mono Book 12";
static char* font = NULL;
typedef enum {
	Bottom,
	Top,
	Left,
	Right
} ScreenSide;
static ScreenSide dockposition = Bottom;
static Bool picomfreezeworkaround = True;
static int dockmonitor = 0;
static char* terminal[2] = { NULL, NULL };
static char* userscript = NULL;
static const char* defaultterminal[] = { "kitty", NULL };
static const char histfile[]        = "/home/tinou/.surf/history.dmenu";
static const char normbordercolor[] = "#444444";
static const char selbordercolor[]  = "#ffffff";
static const char defnormbgcolor[8]     = "#222222";
static const char defnormfgcolor[8]     = "#bbbbbb";
static const char defselbgcolor[8]      = "#005577";
static const char defselfgcolor[8]      = "#eeeeee";
static char normbgcolor[8]; //     = "#222222";
static char normfgcolor[8]; //     = "#bbbbbb";
static char selbgcolor[8]; //      = "#005577";
static char selfgcolor[8]; //      = "#eeeeee";
#define DEFAULT_BORDER_PX 2
static const unsigned int borderpx  = DEFAULT_BORDER_PX;        /* border pixel of windows */
static const unsigned int snap      = 32;       /* snap pixel */
static const unsigned int systraypinning = 0;   /* 0: sloppy systray follows selected monitor, >0: pin systray to monitor X */
static const unsigned int systrayspacing = 2;   /* systray spacing */
static const Bool systraypinningfailfirst = True;   /* True: if pinning fails, display systray on the first monitor, False: display systray on the last monitor*/
static const Bool showsystray       = False;    /* False means no systray */
static const Bool showbar           = True;     /* False means no bar */
static const Bool showdock          = True;
static const Bool topbar            = True;    /* False means bottom bar */
static const Bool centretitle       = True;
static Bool foldtags                = True;
static Bool showtagshortcuts        = False;
static const int windowgap			= 36; /* gap between windows */
static const int focusmonstart		= 0;
static const Bool statusallmonitor  = True;
#define OOFTRAYLEN 5
static const char* outoffocustraysymbol = "X";

#define Button6 (Button1 + 5)
#define Button7 (Button1 + 6)
#define Button8 (Button1 + 7)
#define Button9 (Button1 + 8)
#define Button10 (Button1 + 9)
#define Button11 (Button1 + 10)
#define Button12 (Button1 + 11)

#define SPCTR 0.2
#define CLEAR 0.618
#define TRANS 0.75
#define RDLBL 0.82
#define SUBTL 0.92
#define OPAQU 1.0

static const int layoutaxis[] = {
	1,    /* layout axis: 1 = x, 2 = y; negative values mirror the layout, setting the master area to the right / bottom instead of left / top */
	2,    /* master axis: 1 = x (from left to right), 2 = y (from top to bottom), 3 = z (monocle) */
	2,    /* stack axis:  1 = x (from left to right), 2 = y (from top to bottom), 3 = z (monocle) */
};
/* layout(s) */
static const float mfact      = 0.55; /* factor of master area size [0.05..0.95] */
static int initlayout = 0;
static double barOpacity = CLEAR;

#include "flextile.c"
#include "fibonacci.c"

#define LT(a) a, #a

static const Layout layouts[] = {
	/* symbol     arrange function       win border width   showdock */
	{ "[]=",      LT(tile),              DEFAULT_BORDER_PX, True  },
	{ "[]@",      LT(spiral),            DEFAULT_BORDER_PX, True  },
	{ "[]G",      LT(dwindle),           DEFAULT_BORDER_PX, True  },
	{ "><>",      NULL, "",              DEFAULT_BORDER_PX, True  },
	{ "[M]",      LT(monocle),           0,                 False },
};

enum layout {
	TILE = 0,
	SPIRAL,
	DWINDLE,
	FLOAT,
	MONOCLE,
};

static const Rule defaultrule = 
	/* class , instance , title , tags mask , float , term  , noswl , trnsp , nofcs , nobdr , rh   , mon , remap , preflt , istrans , isfullscreen , showdock , procname , next */
	{   NULL , NULL     , NULL  , anytag    , False , False , False , OPAQU , False , False , True , -1  , NULL  , NULL   , False   , False        , -1       , NULL     , NULL };

static Rule* rules = NULL;

/* helper for spawning shell commands in the pre dwm-5.0 fashion */
#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }

#include "remap.c"
#include "jsonconfig.c"
#include "viewstack.c"
#include "push.c"
#include "rotatemons.c"
#include "misc.c"

#include <X11/XF86keysym.h>

/* key definitions */
#define MODKEY Mod4Mask
static const KeySym modkeysyms[] = { XK_Super_L, XK_Super_R };

#define TAGKEYS(KEY,TAG) \
	{ MODKEY,                       KEY,      view,           {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask,           KEY,      toggleview,     {.ui = 1 << TAG} }, \
	{ MODKEY|ShiftMask,             KEY,      tag,            {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask|ShiftMask, KEY,      toggletag,      {.ui = 1 << TAG} },

#define WORKSPACE(KEY,TAG) \
	{ MODKEY,   KEY,      view,     {.ui = TAG} },

/* commands */
static const char *dclipcmd[] = { "dclip", "paste", "-fn", fallbackfont, "-nb", normbgcolor, "-nf", normfgcolor, "-sb", selbgcolor , "-sf", selfgcolor, NULL };
//static const char *dmenucmd[] = { "./bin/dmenu_run", "-fn", fallbackfont, "-nb", normbgcolor, "-nf", normfgcolor, "-sb", selbgcolor, "-sf", selfgcolor, NULL };
static const char *albertcmd[] = { "/usr/bin/albert", "show", NULL };
static const char *screencmd[]  = { "urxvt", "-e", "screen", "-xRR", NULL };
static const char *clockcmd[] = { "oclock", NULL };
static const char *updatedpicmd[] = { "/bin/sh", "-c", "/home/tinou/hacks/scripts/updateDpi.sh", NULL };
static const char *killclockscmd[] = { "killall", "oclock", NULL };
static const char *rofiwindowcmd[] = { "rofi", "-modi", "combi", "-show", "combi", "-combi-modi", "window,drun" , NULL };
static const Rule clockrule =
	/* class , instance , title , tags mask , float , term  , noswl , trnsp , nofcs , nobdr , rh   , mon , remap , preflt , istrans , isfullscreen , showdock , procname , next */
	{  NULL  , "oclock" , NULL  , alltags   , True  , False , True  , SPCTR , True  , True  , True , -1  , NULL  , NULL   , False   , False        , -1       , NULL     , NULL };

static Key keys[] = {
    /* modifier                     key        function        argument */
    { MODKEY|ControlMask|ShiftMask, XK_l,      spawn,             SHCMD("touch $HOME/.lockX") },
    { MODKEY,                       XK_Menu,   focuslast,      {0} },
    { MODKEY,                       XK_p,      spawn,          {.v = albertcmd } },
    { MODKEY|ShiftMask,             XK_Return, spawnterm,      {0} },
    { MODKEY|ShiftMask,             XK_x,      spawn,          {.v = screencmd } },
    { MODKEY,                       XK_b,      togglebar,      {0} },
    { MODKEY,                       XK_j,      focusstack,     {.i = +1 } },
    { MODKEY,                       XK_k,      focusstack,     {.i = -1 } },
    { MODKEY,                       XK_h,      setmfact,       {.f = -0.05} },
    { MODKEY,                       XK_l,      setmfact,       {.f = +0.05} },
    { MODKEY,                       XK_Return, zoom,           {0} },
    { MODKEY,                       XK_Tab,    view,           {0} },
    { MODKEY,                       XK_space,  setlayout,      {.v = 0 } },
    { Mod1Mask,                     XK_Tab,    spawn,          {.v = rofiwindowcmd } },
    { MODKEY|ShiftMask,             XK_c,      killclient,     {0} },
    { MODKEY,                       XK_t,      setlayout,      {.v = &layouts[TILE]} },
    { MODKEY,                       XK_f,      setlayout,      {.v = &layouts[FLOAT]} },
    { MODKEY,                       XK_m,      setlayout,      {.v = &layouts[MONOCLE]} },
    { MODKEY,                       XK_s,      setlayout,      {.v = &layouts[SPIRAL]} },
    { MODKEY,                       XK_d,      toggledock,     {0} },
	{ MODKEY,                       XK_z,      togglefoldtags, {0} },
    { MODKEY,                       XK_BackSpace, rotatemonitor, {.i = 0} },
    { MODKEY|ShiftMask,             XK_space,  togglefloating, {0} },
    { MODKEY|ControlMask|ShiftMask, XK_space,  allnonfloat,       {0} },
    { MODKEY,                       XK_comma,  focusmon,       {.i = -1 } },
    { MODKEY,                       XK_period, focusmon,       {.i = +1 } },
    { MODKEY|ShiftMask,             XK_comma,  tagmon,         {.i = -1 } },
    { MODKEY|ShiftMask,             XK_period, tagmon,         {.i = +1 } },
    { MODKEY|ControlMask,           XK_j,      pushdown,       {0} },
    { MODKEY|ControlMask,           XK_k,      pushup,         {0} },
    { MODKEY|ControlMask,           XK_c,      spawn,          SHCMD("exec dclip copy") },
    { MODKEY|ControlMask,           XK_v,      spawn,          {.v = dclipcmd } },
    { ControlMask,                  XK_F12,    updatecolors,   SHCMD("exec ~/hacks/scripts/updateDwmColor.sh") },
    { ControlMask|Mod1Mask,         XK_l,      spawn,          SHCMD("exec slimlock") },
    { MODKEY,                       XK_a,      spawn,          SHCMD("kill -s USR1 $(pidof deadd-notification-center)") },
    TAGKEYS(                        XK_1,                      0)
    TAGKEYS(                        XK_2,                      1)
    TAGKEYS(                        XK_3,                      2)
    TAGKEYS(                        XK_4,                      3)
    TAGKEYS(                        XK_5,                      4)
    TAGKEYS(                        XK_6,                      5)
    TAGKEYS(                        XK_7,                      6)
    TAGKEYS(                        XK_8,                      7)
    TAGKEYS(                        XK_9,                      8)
    TAGKEYS(                        XK_0,                      9)
    TAGKEYS(                        XK_minus,                  10)
    TAGKEYS(                        XK_equal,                  11)
    { MODKEY|ShiftMask,             XK_q,      quit,           {0} },
    { MODKEY|ControlMask,           XK_t,      rotatelayoutaxis, {.i = 0} },    /* 0 = layout axis */
    { MODKEY|ControlMask,           XK_m,      rotatelayoutaxis, {.i = 1} },    /* 1 = master axis */
    { MODKEY|ControlMask,           XK_s,      rotatelayoutaxis, {.i = 2} },    /* 2 = stack axis */
    { MODKEY|ControlMask,           XK_Return, mirrorlayout,     {0} },
    { MODKEY|ControlMask,           XK_l,      shiftmastersplit, {.i = -1} },   /* reduce the number of tiled clients in the master area */
    { MODKEY|ControlMask,           XK_h,      shiftmastersplit, {.i = +1} },   /* increase the number of tiled clients in the master area */
    { MODKEY|ControlMask,           XK_Right,  rotatemonitor,  {.i = 0} },
    { MODKEY,                       XK_u,      toggleswallow,    {0} },
};

/* button definitions */
/* click can be ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
static Button buttons[] = {
	/* click                event mask      button          function        argument */
	{ ClkLtSymbol,          0,              Button1,        toggledock,     {0} },
	{ ClkLtSymbol,          0,              Button2,        setlayout,      {.v = &layouts[TILE] } },
	{ ClkLtSymbol,          0,              Button3,        setlayout,      {.v = &layouts[MONOCLE] } },
	{ ClkLtSymbol,          0,              Button4,        viewscroll,     {.f = -0.05 } },
	{ ClkLtSymbol,          0,              Button5,        viewscroll,     {.f = +0.05 } },
	{ ClkWinTitle,          0,              Button2,        killclient,     {0} },
	{ ClkWinTitle,          0,              Button1,        zoom,           {0} },
	{ ClkWinTitle,          0,              Button3,        tagmon,         {.i = +1 } },
	{ ClkWinTitle,          0,              Button4,        opacitychange,  {.f = -0.02 } },
	{ ClkWinTitle,          0,              Button5,        opacitychange,  {.f = +0.02 } },
	{ ClkWinTitle,          0,              Button6,        shiftmastersplit,  {.i = +1} },
	{ ClkWinTitle,          0,              Button7,        shiftmastersplit,  {.i = -1} },
	{ ClkWinTitle,          0,              Button9,        focusstack,  {.i = +1} },
	{ ClkWinTitle,          0,              Button8,        focusstack,  {.i = -1} },
	{ ClkStatusText,        0,              Button1,        spawn,          SHCMD("urxvt -name ghost_terminal -e /home/tinou/hacks/scripts/network_status.sh") },
	{ ClkStatusText,        MODKEY,         Button2,        spawnterm,      {0} },
	{ ClkStatusText,        0,              Button2,        spawn,          SHCMD("qdbus org.kde.kglobalaccel /component/kmix invokeShortcut \"mute\"") },
	{ ClkStatusText,        0,              Button3,        rotatemonitor,  {.i = 1} },
	{ ClkStatusText,        0,              Button4,        spawn,          SHCMD("qdbus org.kde.kglobalaccel /component/kmix invokeShortcut \"increase_volume\"") },
	{ ClkStatusText,        0,              Button5,        spawn,          SHCMD("qdbus org.kde.kglobalaccel /component/kmix invokeShortcut \"decrease_volume\"") },
	{ ClkStatusText,        0,              Button9,        sendselkey,     {.keysym = XK_Left} },
	{ ClkStatusText,        0,              Button8,        sendselkey,     {.keysym = XK_Right} },
	{ ClkClientWin,         MODKEY,         Button1,        movemouse,      {0} },
	{ ClkClientWin,         MODKEY,         Button2,        togglefloating, {0} },
	{ ClkClientWin,         MODKEY,         Button3,        resizemouse,    {0} },
	{ ClkTagBar,            0,              Button1,        view,           {0} },
	{ ClkTagBar,            0,              Button3,        toggleview,     {0} },
	{ ClkTagBar,            0,              Button4,        increasebright, {0} },
	{ ClkTagBar,            0,              Button5,        decreasebright, {0} },
	{ ClkTagBar,            0,              Button9,        togglefoldtags, {0} },
	{ ClkTagBar,            0,              Button8,        tabview,        {.i = -1} },
	{ ClkTagBar,            MODKEY,         Button1,        tag,            {0} },
	{ ClkTagBar,            MODKEY,         Button3,        toggletag,      {0} },
};

#endif
