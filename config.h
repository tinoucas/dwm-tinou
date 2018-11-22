/* See LICENSE file for copyright and license details. */

#ifdef CONFIG_HEAD
/* tagging */
static const char *tags[] = { "main", "chat", "files", "text", "edit", "dl", "trrnt", "music", "misc", "@", "v" };

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
static const unsigned int vtag = 1 << 10;

static const unsigned int anytag = 0;
static const unsigned int alltags = ~0;
#else

/* appearance */
//static const char font[]            = "-*-helvetica-bold-r-*-*-12-*-*-*-*-*-*-1";
//static const char font[]            = "-*-freemono-medium-r-normal-*-*-*-*-*-*-*-*-1";
//static const char font[]            = "-*-dejavu sans condensed-medium-r-*-*-*-*-*-*-*-*-ascii-*";
//static const char font[]            = "-*-clean-medium-r-normal-*-12-*-*-*-*-*-iso8859-1";
//static const char font[]            = "-*-droid sans-medium-r-*-*-12-*-*-*-*-*-ascii-*";
//static const char font[]            = "-*-fixed-medium-r-*-*-13-*-*-*-*-*-*-15";
//static const char font[]            = "-misc-liberation mono-medium-r-*-*-12-*-*-*-*-*-*-*";
static const char font[]			= "Andale Mono Regular 11";
//static const char font[]            = "-*-proggyclean-medium-*-*-*-*-*-*-*-*-*-*-1";
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
static const Bool showsystray       = True;     /* False means no systray */
static const Bool showbar           = True;     /* False means no bar */
static const Bool topbar            = True;    /* False means bottom bar */
static const int windowgap			= 14; /* gap between windows */
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

static const Layout layouts[] = {
	/* symbol     arrange function   window border width */
	{ "[]=",      tile,              DEFAULT_BORDER_PX },
	{ "[]@",      spiral,            DEFAULT_BORDER_PX },
	{ "[]G",      dwindle,           DEFAULT_BORDER_PX },
	{ "><>",      NULL,              DEFAULT_BORDER_PX },
	{ "[M]",      monocle,           0 },
};

enum layout {
	TILE = 0,
	SPIRAL,
	DWINDLE,
	FLOAT,
	MONOCLE,
};

static const Layout *const monoclelt = &layouts[MONOCLE];

#include "viewstack.c"
#include "push.c"
#include "rotatemons.c"
#include "remap.c"
#include "misc.c"

#include <X11/XF86keysym.h>

static const Rule defaultrule = 
	/* class           , instance            , title       , tags mask , float , term  , noswl , trnsp , nofcs , nobdr , rh    , mon , remap  , preflt    , istrans , procname */
    {   NULL           , NULL                , NULL        , anytag    , True  , False , False , OPAQU , False , False , True  , -1  , NULL   , NULL      , False   , NULL };

static const Rule rules[] = {
	/* xprop(1):
	 * 
	 *	WM_CLASS(STRING) = instance, class
	 *	WM_NAME(STRING) = title
	 */
	/* class           , instance            , title       , tags mask , float , term  , noswl , trnsp , nofcs , nobdr , rh    , mon , remap  , preflt    , istrans , procname */
	{ NULL             , NULL                , NULL        , anytag    , False , False , False , OPAQU , False , False , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	{ "veromix"        , NULL                , NULL        , anytag    , True  , False , False , TRANS , False , False , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	{ "Alacritty"      , "Alacritty"         , NULL        , anytag    , False , False , False , CLEAR , False , False , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	{ "kitty"          , "kitty"             , NULL        , anytag    , False , True  , False , OPAQU , False , False , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	{ "kitty"          , "kitty"             , "mon0"      , anytag    , False , True  , False , OPAQU , False , False , True  ,  0  , NULL   , NULL      , False   , NULL      } , 
	{ "kitty"          , "kitty"             , "mon1"      , anytag    , False , True  , False , OPAQU , False , False , True  ,  1  , NULL   , NULL      , False   , NULL      } , 
	{ "URxvt"          , NULL                , NULL        , maintag   , False , False , False , TRANS , False , False , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	{ "URxvt"          , "screen"            , NULL        , maintag   , False , False , False , TRANS , False , False , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	{ NULL             , "ghost_terminal"    , NULL        , anytag    , False , False , False , OPAQU , True  , False , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	{ NULL             , "xterm"             , NULL        , anytag    , False , False , False , TRANS , False , False , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	{ "Gimp"           , NULL                , NULL        , edittag   , False , False , False , OPAQU , False , False , False , -1  , NULL   , NULL      , False   , NULL      } , 
	{ "Firefox"        , NULL                , NULL        , webtag    , False , False , False , OPAQU , False , False , True  , -1  , chrome , monoclelt , False   , NULL      } , 
	{ "vivaldi-stable" , NULL                , NULL        , webtag    , False , False , False , OPAQU , False , False , True  , -1  , chrome , NULL      , False   , NULL      } , 
	{ NULL             , "Download"          , NULL        , webtag    , False , False , False , OPAQU , False , False , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	{ NULL             , "Navigator"         , NULL        , webtag    , False , False , False , OPAQU , False , False , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	{ "Gran Paradiso"  , NULL                , NULL        , webtag    , False , False , False , OPAQU , False , False , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	{ "Opera"          , NULL                , NULL        , webtag    , False , False , False , OPAQU , False , False , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	{ "Google-chrome"  , "google-chrome"     , NULL        , webtag    , False , False , False , OPAQU , False , False , True  , -1  , chrome , NULL      , False   , NULL      } , 
	{ "Yandex-browser" , "yandex-browser"    , NULL        , webtag    , False , False , False , OPAQU , False , False , True  , -1  , chrome , monoclelt , False   , NULL      } , 
	{ "Slimjet"        , "slimjet"           , NULL        , webtag    , False , False , False , OPAQU , False , False , True  , -1  , chrome , monoclelt , False   , NULL      } , 
	{ NULL             , "chrome_app_list"   , NULL        , anytag    , True  , False , False , OPAQU , False , False , True  , -1  , chrome , NULL      , False   , NULL      } , 
	{ "Chromium"       , "chromium"          , NULL        , webtag    , False , False , False , OPAQU , False , False , True  , -1  , chrome , NULL      , False   , NULL      } , 
	{ NULL             , "Pidgin"            , NULL        , chattag   , False , False , False , TRANS , False , False , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	{ NULL             , "sonata"            , NULL        , musictag  , False , False , False , TRANS , False , False , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	{ NULL             , "ario"              , NULL        , musictag  , False , False , False , TRANS , False , False , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	{ "Gmpc"           , NULL                , NULL        , musictag  , False , False , False , TRANS , False , False , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	{ "Shredder"       , NULL                , NULL        , chattag   , False , False , False , TRANS , False , False , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	{ NULL             , "screen"            , NULL        , maintag   , False , False , False , TRANS , False , False , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	{ "feh"            , NULL                , NULL        , anytag    , True  , False , False , OPAQU , False , False , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	{ "feh"            , NULL                , "mon0"      , vtag      , True  , False , True  , OPAQU , False , False , True  , 0   , mpv    , NULL      , False   , NULL      } , 
	{ "feh"            , NULL                , "mon1"      , vtag      , True  , False , True  , OPAQU , False , False , True  , 1   , mpv    , NULL      , False   , NULL      } , 
	{ NULL             , "savebox"           , NULL        , anytag    , True  , False , False , TRANS , False , False , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	{ "Xfe"            , NULL                , NULL        , filestag  , False , False , False , TRANS , False , False , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	{ NULL             , "ROX-Filer"         , NULL        , filestag  , False , False , False , TRANS , False , False , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	{ NULL             , "spacefm"           , NULL        , filestag  , False , False , False , TRANS , False , False , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	{ NULL             , "thunar"            , NULL        , filestag  , False , False , False , TRANS , False , False , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	{ NULL             , "dolphin"           , NULL        , filestag  , False , False , False , TRANS , False , False , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	/*{ NULL             , "dolphin"           , NULL        , filestag  , False , False , False , TRANS , False , False , True  , -1  , NULL   , NULL      , True    , NULL      } , */
	{ NULL             , "pantheon-files"    , NULL        , filestag  , False , False , False , TRANS , False , False , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	{ NULL             , NULL                , "Rename"    , anytag    , True  , False , False , TRANS , False , False , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	{ NULL             , NULL                , "Delete"    , anytag    , True  , False , False , TRANS , False , False , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	{ NULL             , NULL                , "Copy"      , anytag    , True  , False , False , TRANS , False , False , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	{ NULL             , NULL                , "Move"      , anytag    , True  , False , False , TRANS , False , False , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	{ NULL             , NULL                , "Mount"     , anytag    , True  , False , False , TRANS , False , False , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	{ NULL             , NULL                , "Renommer"  , anytag    , True  , False , False , TRANS , False , False , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	{ NULL             , NULL                , "Supprimer" , anytag    , True  , False , False , TRANS , False , False , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	{ NULL             , NULL                , "Copier"    , anytag    , True  , False , False , TRANS , False , False , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	{ NULL             , NULL                , "Déplacer"  , anytag    , True  , False , False , TRANS , False , False , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	{ NULL             , NULL                , "Monter"    , anytag    , True  , False , False , TRANS , False , False , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	{ "Audacious"      , NULL                , NULL        , musictag  , False , False , False , TRANS , False , False , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	{ "MPlayer"        , NULL                , NULL        , anytag    , True  , False , True  , OPAQU , False , False , True  , -1  , mpv    , monoclelt , False   , NULL      } , 
	{ "sView"          , "sView"             , NULL        , vtag      , False , False , True  , OPAQU , False , False , True  , -1  , sView  , monoclelt , False   , NULL      } , 
	{ "qtwebflix"      , "qtwebflix"         , NULL        , vtag      , False , False , True  , OPAQU , False , False , True  , -1  , mpv    , monoclelt , False   , NULL      } , 
	{ "mpv"            , NULL                , NULL        , anytag    , True  , False , True  , OPAQU , False , False , True  , -1  , mpv    , monoclelt , False   , NULL      } , 
	{ "mpv"            , NULL                , "mon0"      , anytag    , True  , False , True  , OPAQU , False , False , True  , 0   , mpv    , monoclelt , False   , NULL      } , 
	{ "mpv"            , NULL                , "mon1"      , anytag    , True  , False , True  , OPAQU , False , False , True  , 1   , mpv    , monoclelt , False   , NULL      } , 
	{ "Vlc"            , NULL                , NULL        , anytag    , False , False , False , OPAQU , False , False , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	{ "Gcalctool"      , NULL                , NULL        , anytag    , True  , False , False , OPAQU , False , False , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	{ NULL             , "gqmpeg"            , NULL        , musictag  , True  , False , False , OPAQU , False , False , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	{ "GQmpeg"         , "playlist"          , NULL        , chattag   , False , False , False , OPAQU , False , False , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	{ NULL             , "TzClock"           , NULL        , alltags   , True  , False , False , TRANS , True  , True  , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	{ "Guimup"         , "guimup"            , NULL        , musictag  , False , False , False , OPAQU , False , False , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	{ NULL             , "uzbl-core"         , NULL        , webtag    , False , False , False , OPAQU , False , False , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	{ NULL             , "gvim"              , NULL        , texttag   , False , False , False , TRANS , False , False , False , -1  , NULL   , NULL      , False   , NULL      } , 
	{ NULL             , "vim"               , NULL        , texttag   , False , False , False , TRANS , False , False , False , -1  , NULL   , NULL      , False   , NULL      } , 
	{ "Sublime_text"   , NULL                , NULL        , texttag   , False , False , False , TRANS , False , False , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	{ "oni"            , "oni"               , NULL        , texttag   , False , False , False , TRANS , False , False , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	{ NULL             , "MixVibes Cross"    , NULL        , misctag   , False , False , False , OPAQU , False , False , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	{ NULL             , "Cross Preferences" , NULL        , misctag   , True  , False , False , OPAQU , False , False , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	{ "Evince"         , NULL                , NULL        , edittag   , False , False , False , OPAQU , False , False , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	{ "FBReader"       , NULL                , NULL        , edittag   , False , False , False , TRANS , False , False , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	{ NULL             , "stalonetray"       , NULL        , alltags   , True  , False , False , OPAQU , True  , True  , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	{ "Display"        , NULL                , NULL        , maintag   , True  , False , False , OPAQU , True  , True  , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	{ "broken"         , NULL                , "Renoise"   , edittag   , False , False , False , OPAQU , False , False , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	{ "emulator64-arm" , NULL                , NULL        , misctag   , True  , False , False , OPAQU , False , False , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	{ "Deadbeef"       , "deadbeef"          , NULL        , musictag  , False , False , False , TRANS , False , False , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	{ NULL             , "Telegram"          , NULL        , chattag   , False , False , False , OPAQU , False , False , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	{ "Steam"          , "Steam"             , NULL        , misctag   , False , False , False , OPAQU , False , False , True  , -1  , NULL   , monoclelt , False   , NULL      } , 
	{ "Steam.exe"      , "Steam.exe"         , NULL        , misctag   , False , False , False , OPAQU , False , True  , False , -1  , NULL   , NULL      , False   , NULL      } , 
	{ "VPN Unlimited"  , "vpn-unlimited"     , NULL        , misctag   , True  , False , False , OPAQU , False , False , True  , 1   , NULL   , NULL      , False   , NULL      } , 
	{ "qBittorrent"    , "qbittorrent"       , NULL        , trrnttag  , False , False , False , OPAQU , False , False , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	{ "JDownloader"    , NULL                , NULL        , dltag     , False , False , False , OPAQU , False , False , True  , -1  , NULL   , NULL      , False   , NULL      } , 
	/*{ "JDownloader"    , NULL                , NULL        , dltag     , True  , False , False , OPAQU , False , False , True  , -1  , NULL   , NULL      , True    , NULL      } , */
	{ NULL             , NULL                , NULL        , musictag  , False , False , False , SUBTL , False , False , True  , -1  , NULL   , NULL      , False   , "spotify" } , 
    { "broken"         , "broken"            , NULL        , vtag      , False , False , True  , OPAQU , False , False , True  , 1   , NULL   , monoclelt , False   , "yandex_browser" } , 
};

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
static const char *dmenucmd[] = { "./bin/dmenu_run", "-fn", font, "-nb", normbgcolor, "-nf", normfgcolor, "-sb", selbgcolor, "-sf", selfgcolor, NULL };
//static const char *dmenucmd[] = { "dmenu_run", "-fn", font, "-nb", normbgcolor, "-nf", normfgcolor, "-sb", selbgcolor, "-sf", selfgcolor, NULL };
static const char *termcmd[]  = { "kitty", NULL };
static const char *screencmd[]  = { "urxvt", "-e", "screen", "-xRR", NULL };
static const char *clockcmd[] = { "oclock", NULL };
static const char *killclockscmd[] = { "killall", "oclock", NULL };
static const Rule clockrule =
	/* class           , instance            , title       , tags mask , float , term  , noswl , trnsp , nofcs , nobdr , rh    , mon , remap  , preflt    , istrans , procname */
    {  NULL            , "oclock"            , NULL        , alltags   , True  , False , True  , SPCTR , True  , True  , True  ,-1   , NULL   , NULL      , False   , NULL };

static Key keys[] = {
    /* modifier                     key        function        argument */
    { MODKEY|ShiftMask,             XK_l,      spawn,             SHCMD("$HOME/hacks/scripts/lockX.sh") },
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
    { MODKEY,                       XK_BackSpace, rotatemonitor, {.i = 0} },
    { MODKEY,                       XK_space,  rotatemonitor,  {.i = 1} },
    { MODKEY|ShiftMask,             XK_space,  togglefloating, {0} },
    { MODKEY|ControlMask|ShiftMask, XK_space,  allnonfloat,       {0} },
    //{ MODKEY,                       XK_0,      view,           {.ui = ~0 } },
    //{ MODKEY|ShiftMask,             XK_0,      tag,            {.ui = ~0 } },
    { MODKEY,                       XK_comma,  focusmon,       {.i = -1 } },
    { MODKEY,                       XK_period, focusmon,       {.i = +1 } },
    { MODKEY|ShiftMask,             XK_comma,  tagmon,         {.i = -1 } },
    { MODKEY|ShiftMask,             XK_period, tagmon,         {.i = +1 } },
    { MODKEY|ControlMask,           XK_j,      pushdown,       {0} },
    { MODKEY|ControlMask,           XK_k,      pushup,         {0} },
    { MODKEY|ControlMask,           XK_c,      spawn,          SHCMD("exec dclip copy") },
    { MODKEY|ControlMask,           XK_v,      spawn,          {.v = dclipcmd } },
    { ControlMask,                  XK_F12,    updatecolors,   SHCMD("exec ~/hacks/scripts/updateDwmColor.sh") },
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
    TAGKEYS(                        XK_ampersand,              0)
    TAGKEYS(                        XK_eacute,                 1)
    TAGKEYS(                        XK_quotedbl,               2)
    TAGKEYS(                        XK_apostrophe,             3)
    TAGKEYS(                        XK_parenleft,              4)
    TAGKEYS(                        XK_minus,                  5)
    TAGKEYS(                        XK_egrave,                 6)
    TAGKEYS(                        XK_underscore,             7)
    TAGKEYS(                        XK_ccedilla,               8)
    WORKSPACE(                      XK_a,                      chattag | 1 << 5)
    WORKSPACE(                      XK_w,                      maintag | texttag)
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
    { MODKEY|ControlMask,           XK_Right,  rotatemonitor,  {.i = 0} },
    { MODKEY,                       XK_u,      toggleswallow,    {0} },
};

/* button definitions */
/* click can be ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
static Button buttons[] = {
	/* click                event mask      button          function        argument */
	{ ClkLtSymbol,          0,              Button1,        setlayout,      {0} },
	{ ClkLtSymbol,          0,              Button3,        setlayout,      {.v = monoclelt} },
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
	{ ClkStatusText,        0,              Button2,        spawn,          SHCMD("/home/tinou/hacks/scripts/Volume.sh mute") },
	{ ClkStatusText,        0,              Button3,        rotatemonitor,  {.i = 1} },
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

#endif
