/* See LICENSE file for copyright and license details. */

#ifdef CONFIG_HEAD
/* tagging */
static char **tags;
static char *tagkeys[32];
static unsigned int tagkeysmod = 0;
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
static int dockmonitor = 0;
static char* terminal[2] = { NULL, NULL };
static char* userscript = NULL;
static const char* defaultterminal[] = { "kitty", NULL };
static const char normbordercolor[] = "#222222";
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
#include "varimono.c"
#include "fibonacci.c"

#define LT(a) a, #a

static const Layout layouts[] = {
	/* symbol     arrange function       win border width   showdock */
	{ "{-}",      LT(varimono),          DEFAULT_BORDER_PX, True  },
	{ "[]=",      LT(tile),              DEFAULT_BORDER_PX, True  },
	{ "[]@",      LT(spiral),            DEFAULT_BORDER_PX, True  },
	{ "[]G",      LT(dwindle),           DEFAULT_BORDER_PX, True  },
	{ "><>",      NULL, "",              DEFAULT_BORDER_PX, True  },
	{ "[M]",      LT(monocle),           0,                 False },
};

enum layout {
	VARIMONO = 0,
	TILE,
	SPIRAL,
	DWINDLE,
	FLOAT,
	MONOCLE,
};

static const Rule defaultrule = 
	/* class , instance , title , tags mask , float , center, term  , trnsp , nofcs , nobdr , rh   , mon , remap , preflt , istrans , isfullscreen , showdock , procname , picomfreeze, next */
	{   NULL , NULL     , NULL  , anytag    , False , False , False , OPAQU , False , False , True , -1  , NULL  , NULL   , False   , False        , -1       , NULL     ,       False, NULL };

static Rule* rules = NULL;

/* helper for spawning shell commands in the pre dwm-5.0 fashion */
static Key* keys = NULL;
static Button* buttons = NULL;

#include "remap.c"
#include "viewstack.c"
#include "push.c"
#include "rotatemons.c"
#include "misc.c"
#include "jsonconfig.c"

#include <X11/XF86keysym.h>

/* key definitions */
static const KeySym modkeysyms[] = { XK_Super_L, XK_Super_R };

/* commands */
static const char *clockcmd[] = { "oclock", NULL };
static const char *updatedpicmd[] = { "/bin/sh", "-c", "/home/tinou/hacks/scripts/updateDpi.sh", NULL };
static const char *killclockscmd[] = { "killall", "oclock", NULL };
static const Rule clockrule =
	/* class , instance , title , tags mask , float , center, term  , trnsp , nofcs , nobdr , rh   , mon , remap , preflt , istrans , isfullscreen , showdock , procname , picomfreeze, next */
	{  NULL  , "oclock" , NULL  , alltags   , True  , False , False , SPCTR , True  , True  , True , -1  , NULL  , NULL   , False   , False        , -1       , NULL     , False,       NULL };

#endif
