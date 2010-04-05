/* See LICENSE file for copyright and license details. */

/* appearance */
static const char font[]            = "-windows-proggyclean-medium-r-normal--13-80-96-96-c-70-iso8859-1";
static const char normbordercolor[] = "#262626";
static const char normbgcolor[]     = "#262626";
static const char normfgcolor[]     = "#b0b4ac";
static const char selbordercolor[]  = "#ff0000";
static const char selbordercolorsingle[]  = "#000000";
static const char selbgcolor[]      = "#464646";
static const char selfgcolor[]      = "#d3d7cf";
static const unsigned int borderpx  = 2;        /* border pixel of windows */
static const unsigned int snap      = 32;       /* snap pixel */
static const Bool showbar           = True;     /* False means no bar */
static const Bool topbar            = False;     /* False means bottom bar */

/* tagging */
static const char *tags[] = { "1", "2", "3", "4", "5", "6", "7", "8", "9" };

static const Rule rules[] = {
	/*	class				instance		title			tags mask	isfloating	monitor */
	{   "URxvt",            NULL,           NULL,           0,          False,      -1 },
	{	"Gimp",				NULL,			NULL,			1 << 4,		False,		 0 },
	{	"Firefox",			NULL,			NULL,			1 << 8,		False,		 0 },
	{	NULL,				"Navigator",	NULL,			1 << 8,		False,		 0 },
	{	"Gran Paradiso",	NULL,			NULL,			1 << 8,		True,		 0 },
	{	"Opera",			NULL,			NULL,			1 << 8,		False,		-1 },
	{	"Google-chrome",	"google-chrome",NULL,			1 << 8,		False,		 0 },
	{	NULL,				"Pidgin",		NULL,			1 << 1,		False,		-1 },
	{	NULL,				"sonata",		NULL,			1 << 1,		False,		-1 },
	{	NULL,				"screen",		NULL,			1,			False,		-1 },
	{	"feh",				NULL,			NULL,			0,			True,		-1 },
	{	NULL,				"savebox",		NULL,			0,			True,		-1 },
	{	NULL,				NULL,			"Rename",		0,			True,		-1 },
	{	NULL,				NULL,			"Delete",		0,			True,		-1 },
	{	NULL,				NULL,			"Copy",			0,			True,		-1 },
	{	NULL,				NULL,			"Move",			0,			True,		-1 },
	{	NULL,				NULL,			"Mount",		0,			True,		-1 },
	{	NULL,				NULL,			"Renommer",		0,			True,		-1 },
	{	NULL,				NULL,			"Supprimer",	0,			True,		-1 },
	{	NULL,				NULL,			"Copier",		0,			True,		-1 },
	{	NULL,				NULL,			"Déplacer",		0,			True,		-1 },
	{	NULL,				NULL,			"Monter",		0,			True,		-1 },
	{	"Audacious",		NULL,			NULL,			1 << 5,		True,		-1 },
	{	"MPlayer",			NULL,			NULL,			0,			True,		-1 },
	{	"Gcalctool",		NULL,			NULL,			0,			True,		-1 },
	{	NULL,				"gqmpeg",		NULL,			1 << 5,		True,		-1 },
	{	"GQmpeg",			"playlist",		NULL,			1 << 1,		False,		-1 },
	{	NULL,				"oclock",		NULL,			1 << 5,		True,		-1 },
	{	"Guimup",			"guimup",		NULL,			1 << 5,		False,		-1 },
};

/* layout(s) */
static const float mfact      = 0.55; /* factor of master area size [0.05..0.95] */
static /*const*/ Bool resizehints = True; /* False means respect size hints in tiled resizals */

#include "tinou.c"
#include "ctrlmap.c"
#include "push.c"
#include "bstack.c"
#include "bstackhoriz.c"

static const Layout layouts[] = {
	/* symbol     arrange function */
	{ "[]=",      tile },    /* first entry is default */
	{ "><>",      NULL },    /* no layout function means floating behavior */
	{ "[M]",      monocle },
	{ "TTT",      bstack },
	{ "===",      bstackhoriz },
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

/* commands */
static const char *dmenucmd[] = { "dmenu_run", "-b", "-fn", font, "-nb", normbgcolor, "-nf", normfgcolor, "-sb", selbgcolor, "-sf", selfgcolor, NULL };
static const char *dclipcmd[] = { "dclip", "paste", "-fn", font, "-nb", normbgcolor, "-nf", normfgcolor, "-sb", selbgcolor , "-sf", selfgcolor, NULL };
static const char *termcmd[]  = { "urxvt", NULL };

static Key keys[] = {
	/* modifier                     key        function        argument */
	{ MODKEY,                       XK_Left,   kbmvresize,     {.v = (int []){ -40, 0, 0, 0 } } },
	{ MODKEY,                       XK_Up,     kbmvresize,     {.v = (int []){ 0, -40, 0, 0 } } },
	{ MODKEY,                       XK_Right,  kbmvresize,     {.v = (int []){ 40, 0, 0, 0 } } },
	{ MODKEY,                       XK_Down,   kbmvresize,     {.v = (int []){ 0, 40, 0, 0 } } },
	{ MODKEY|ShiftMask,             XK_Left,   kbmvresize,     {.v = (int []){ 0, 0, -40, 0 } } },
	{ MODKEY|ShiftMask,             XK_Up,     kbmvresize,     {.v = (int []){ 0, 0, 0, -40 } } },
	{ MODKEY|ShiftMask,             XK_Right,  kbmvresize,     {.v = (int []){ 0, 0, 40, 0 } } },
	{ MODKEY|ShiftMask,             XK_Down,   kbmvresize,     {.v = (int []){ 0, 0, 0, 40 } } },
	{ ControlMask,                  XK_dollar,   sendbracketright,{0} },
	{ ControlMask,                  XK_asterisk, sendbackslash,   {0} },
	{ ControlMask,           XK_dead_circumflex, sendbracketleft, {0} },
	{ MODKEY,                       XK_r,      spawn,          {.v = dmenucmd } },
	{ MODKEY,                       XK_p,      spawn,          {.v = dmenucmd } },
	{ MODKEY|ShiftMask,             XK_Return, spawn,          {.v = termcmd } },
	{ MODKEY,                       XK_b,      togglebar,      {0} },
	{ MODKEY,                       XK_j,      focusstack,     {.i = +1 } },
	{ MODKEY,                       XK_k,      focusstack,     {.i = -1 } },
	{ MODKEY,                       XK_h,      setmfact,       {.f = -0.05} },
	{ MODKEY,                       XK_l,      setmfact,       {.f = +0.05} },
	{ MODKEY|ControlMask,           XK_j,      pushdown,       {0} },
	{ MODKEY|ControlMask,           XK_k,      pushup,         {0} },
	{ MODKEY,                       XK_Return, zoom,           {0} },
	{ MODKEY,                       XK_Tab,    view,           {0} },
	{ MODKEY|ShiftMask,             XK_c,      killclient,     {0} },
	{ MODKEY,                       XK_d,      setlayout,      {.v = &layouts[0]} },
	{ MODKEY,                       XK_f,      setlayout,      {.v = &layouts[1]} },
	{ MODKEY,                       XK_m,      setlayout,      {.v = &layouts[2]} },
	{ MODKEY,                       XK_t,      setlayout,      {.v = &layouts[3]} },
	{ MODKEY|ShiftMask,             XK_h,      setlayout,      {.v = &layouts[4]} },
	{ MODKEY,                       XK_space,  toggleview,     {.ui = 1 << 5} },
	{ MODKEY|ShiftMask,             XK_space,  togglefloating, {0} },
	{ MODKEY|ControlMask,           XK_c,      spawn,          SHCMD("exec dclip copy") },
	{ MODKEY|ControlMask,           XK_v,      spawn,          {.v = dclipcmd } },
	{ MODKEY|ControlMask,           XK_space,  rhtoggle,       {0} },
	{ MODKEY,                       XK_0,      view,           {.ui = ~0 } },
	{ MODKEY|ShiftMask,             XK_0,      tag,            {.ui = ~0 } },
	{ MODKEY,                       XK_Escape, focusmon,       {.i = -1 } },
	{ MODKEY,                       XK_comma,  focusmon,       {.i = -1 } },
	{ MODKEY,                       XK_semicolon,  focusmon,       {.i = +1 } },
	{ MODKEY|ShiftMask,             XK_comma,  tagmon,         {.i = -1 } },
	{ MODKEY|ShiftMask,             XK_semicolon,  tagmon,         {.i = +1 } },
	TAGKEYS(                        XK_1,                      0)
	TAGKEYS(                        XK_2,                      1)
	TAGKEYS(                        XK_3,                      2)
	TAGKEYS(                        XK_4,                      3)
	TAGKEYS(                        XK_5,                      4)
	TAGKEYS(                        XK_6,                      5)
	TAGKEYS(                        XK_7,                      6)
	TAGKEYS(                        XK_8,                      7)
	TAGKEYS(                        XK_9,                      8)
	{ MODKEY,                       XK_agrave,      view,           {.ui = ~0 } },
	{ MODKEY|ShiftMask,             XK_agrave,      tag,            {.ui = ~0 } },
	TAGKEYS(                        XK_ampersand,                       0)
	TAGKEYS(                        XK_eacute,                          1)
	TAGKEYS(                        XK_quotedbl,                        2)
	TAGKEYS(                        XK_apostrophe,                      3)
	TAGKEYS(                        XK_parenleft,                       4)
	TAGKEYS(                        XK_minus,                           5)
	TAGKEYS(                        XK_egrave,                          6)
	TAGKEYS(                        XK_underscore,                      7)
	TAGKEYS(                        XK_ccedilla,                        8)
	{ MODKEY|ShiftMask,             XK_q,      quit,           {0} },
};

/* button definitions */
/* click can be a tag number (starting at 0),
 * ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
static Button buttons[] = {
	/* click                event mask      button          function        argument */
	{ ClkLtSymbol,          0,              Button1,        setlayout,      {0} },
	{ ClkLtSymbol,          0,              Button3,        setlayout,      {.v = &layouts[2]} },
	{ ClkWinTitle,          0,              Button2,        killclient,     {0} },
	{ ClkWinTitle,          0,              Button3,        zoom,           {0} },
	{ ClkWinTitle,          0,              Button4,        ttbarclick,     {.f = -0.05 } },
	{ ClkWinTitle,          0,              Button5,        ttbarclick,     {.f = +0.05 } },
	{ ClkStatusText,        MODKEY,         Button2,        spawn,          {.v = termcmd } },
	{ ClkStatusText,        0,              Button1,        toggleview,     {.ui = 1 << 5} },
	{ ClkStatusText,        0,              Button4,        spawn,          SHCMD("/home/tinou/hack/scripts/Volume.sh up") },
	{ ClkStatusText,        0,              Button5,        spawn,          SHCMD("/home/tinou/hack/scripts/Volume.sh down") },
	{ ClkStatusText,        0,              Button2,        spawn,           SHCMD("/home/tinou/hack/scripts/Volume.sh mute") },
	{ ClkClientWin,         MODKEY,         Button1,        movemouse,      {0} },
	{ ClkClientWin,         MODKEY,         Button2,        togglefloating, {0} },
	{ ClkClientWin,         MODKEY,         Button3,        resizemouse,    {0} },
	{ ClkTagBar,            0,              Button1,        view,           {0} },
	{ ClkTagBar,            0,              Button3,        toggleview,     {0} },
	{ ClkTagBar,            MODKEY,         Button1,        tag,            {0} },
	{ ClkTagBar,            MODKEY,         Button3,        toggletag,      {0} },
};
