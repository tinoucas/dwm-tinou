/* See LICENSE file for copyright and license details. */

/* appearance */
static const char font[]            = "-windows-proggyclean-medium-r-normal--13-80-96-96-c-70-iso8859-1";
/*
 *static const char normbordercolor[] = "#aaaaaa";
 *static const char normbgcolor[]     = "#eeeeee";
 *static const char normfgcolor[]     = "#222222";
 *static const char selbordercolor[]  = "#883737";
 *static const char selbgcolor[]      = "#506070";
 *static const char selfgcolor[]      = "#ffffff";
 */
static const char normbordercolor[] = "#262626";
static const char normbgcolor[]     = "#262626";
static const char normfgcolor[]     = "#b0b4ac";
static const char selbordercolor[]  = "#ff0000";
static const char selbgcolor[]      = "#464646";
static const char selfgcolor[]      = "#d3d7cf";
static const char urgentbordercolor[] = "#ff8a00";
static unsigned int borderpx        = 1;        /* border pixel of windows */
static unsigned int snap            = 32;       /* snap pixel */
static Bool showbar                 = True;     /* False means no bar */
static Bool topbar                  = True;     /* False means bottom bar */
static Bool readin                  = True;     /* False means do not read stdin */

/* tagging */
static const char tags[][MAXTAGLEN] = { "1", "2", "3", "4", "5", "6", "7", "8", "www" };
static unsigned int tagset[] = {1, ~1}; /* after start, first tag is selected */

static Rule rules[] = {
	/*	class				instance		title			tags mask	isfloating	*/
	{	"Gimp",				NULL,			NULL,			1 << 4,		Floating		},
	{	"Firefox",			NULL,			NULL,			1 << 8,		Normal			},
	{	NULL,				"Navigator",	NULL,			1 << 8,		False			},
	{	"Gran Paradiso",	NULL,			NULL,			1 << 8,		False			},
	{	"Opera",			NULL,			NULL,			1 << 8,		Normal			},
	{	NULL,				"pidgin",		NULL,			1 << 1,		Normal			},
	{	NULL,				"sonata",		NULL,			1 << 1,		Normal			},
	{	NULL,				"screen",		NULL,			1,			Normal			},
	{	"feh",				NULL,			NULL,			0,			Floating		},
	{	NULL,				"savebox",		NULL,			0,			Floating		},
	{	NULL,				NULL,			"Rename",		0,			Floating		},
	{	NULL,				NULL,			"Delete",		0,			Floating		},
	{	NULL,				NULL,			"Copy",			0,			Floating		},
	{	NULL,				NULL,			"Move",			0,			Floating		},
	{	NULL,				NULL,			"Mount",		0,			Floating		},
	{	NULL,				NULL,			"Renommer",		0,			Floating		},
	{	NULL,				NULL,			"Supprimer",	0,			Floating		},
	{	NULL,				NULL,			"Copier",		0,			Floating		},
	{	NULL,				NULL,			"Déplacer",		0,			Floating		},
	{	NULL,				NULL,			"Monter",		0,			Floating		},
	{	"Audacious",		NULL,			NULL,			1 << 5,		Floating		},
	{	"MPlayer",			NULL,			NULL,			0,			Floating		},
	{	NULL,				"gqmpeg",		NULL,			1 << 5,		Floating		},
	{	"GQmpeg",			"playlist",		NULL,			1 << 1,		Normal			},
	{	"Conky",			NULL,			NULL,			1 << 5,		NoFocus			},
	{	NULL,				"oclock",		NULL,			~0,			NoFocus|Floating},
};

/* layout(s) */
static float mfact      = 0.55; /* factor of master area size [0.05..0.95] */
//static Bool resizehints = False; [> False means respect size hints in tiled resizals <]
static Bool resizehints[LENGTH(tags) + 1];


#define NMASTER 1
#include "ntile.c"

static Layout layouts[] = {
	/* symbol	arrange function */
	{ "[=]=",	ntile },   /* first entry is default */
	{ "[-]=",	tile },
	{ "><(°>",	NULL },    /* no layout function means floating behavior */
	{ "[<O>]",	monocle },
	{ "[-]/",	tileo },
};

/* Custom functions declarations */ 
static void ttbarclick(const Arg *arg);
static void kbmvresize(const Arg *arg);
static void rhtoggle(const Arg *arg);
int countvisible(Client *c);
static void showmpdtoggle(const Arg *arg);
static void updatewindow(const Arg *arg);

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
static const char *dmenucmd[] = { "dmenu_run", "-fn", font, "-nb", normbgcolor, "-nf", normfgcolor, "-sb", selbgcolor, "-sf", selfgcolor, NULL };
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
	{ MODKEY,                       XK_r,      spawn,          {.v = dmenucmd } },
	{ MODKEY,                       XK_p,      spawn,          {.v = dmenucmd } },
	{ MODKEY|ShiftMask,             XK_Return, spawn,          {.v = termcmd } },
	{ MODKEY,                       XK_b,      togglebar,      {0} },
	{ MODKEY,                       XK_j,      focusstack,     {.i = +1 } },
	{ MODKEY,                       XK_k,      focusstack,     {.i = -1 } },
	{ MODKEY|ShiftMask,             XK_k,      setnmaster,     {.i = -1 } },
	{ MODKEY|ShiftMask,             XK_n,      setnmaster,     {.i = 0  } },
	{ MODKEY|ShiftMask,             XK_j,      setnmaster,     {.i = +1 } },
	{ MODKEY,                       XK_g,      updatewindow,   {0} },
	{ MODKEY,                       XK_h,      setmfact,       {.f = -0.05} },
	{ MODKEY,                       XK_l,      setmfact,       {.f = +0.05} },
	{ MODKEY,                       XK_Return, zoom,           {0} },
	{ MODKEY,                       XK_Tab,    view,           {0} },
	{ MODKEY|ShiftMask,             XK_c,      killclient,     {0} },
	{ MODKEY,                       XK_d,      setlayout,      {.v = &layouts[0]} },
	{ MODKEY,                       XK_t,      setlayout,      {.v = &layouts[1]} },
	{ MODKEY,                       XK_f,      setlayout,      {.v = &layouts[2]} },
	{ MODKEY,                       XK_m,      setlayout,      {.v = &layouts[3]} },
	{ MODKEY,                       XK_o,      setlayout,      {.v = &layouts[4]} },
	{ MODKEY,                       XK_space,  setlayout,      {0} },
	{ MODKEY|ControlMask,           XK_space,  rhtoggle,       {0} },
	{ MODKEY|ShiftMask,             XK_space,  togglefloating, {0} },
	{ MODKEY|ControlMask,           XK_c,      spawn,          SHCMD("exec dclip copy") },
	{ MODKEY|ControlMask,           XK_v,      spawn,          {.v = dclipcmd } },
	{ MODKEY,                       XK_0,      view,           {.ui = ~0 } },
	{ MODKEY|ShiftMask,             XK_0,      tag,            {.ui = ~0 } },
	TAGKEYS(                        XK_1,                               0)
	TAGKEYS(                        XK_2,                               1)
	TAGKEYS(                        XK_3,                               2)
	TAGKEYS(                        XK_4,                               3)
	TAGKEYS(                        XK_5,                               4)
	TAGKEYS(                        XK_6,                               5)
	TAGKEYS(                        XK_7,                               6)
	TAGKEYS(                        XK_8,                               7)
	TAGKEYS(                        XK_9,                               8)
	{ MODKEY,                       XK_agrave,          view,           {.ui = ~0 } },
	{ MODKEY|ShiftMask,             XK_agrave,          tag,            {.ui = ~0 } },
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
	{ ClkLtSymbol,          0,              Button1,        setlayout,      {.v = &layouts[0]} },
	{ ClkLtSymbol,          0,              Button2,        setlayout,      {.v = &layouts[4]} },
	{ ClkLtSymbol,          0,              Button3,        setlayout,      {.v = &layouts[3]} },
	{ ClkWinTitle,          0,              Button2,        spawn,           SHCMD("/home/tinou/hack/scripts/remote-screen-wget.sh \"`/home/tinou/bin/sselp`\"") },
	//{ ClkStatusText,        0,              Button2,        spawn,          {.v = termcmd } },
	{ ClkLtSymbol,          0,              Button4,        focusstack,     {.i = -1 } },
	{ ClkLtSymbol,          0,              Button5,        focusstack,     {.i = +1 } },
	{ ClkWinTitle,          0,              Button1,        zoom,           {0} },
	{ ClkWinTitle,          0,              Button3,        togglefloating, {0} },
	{ ClkWinTitle,          0,              Button4,        ttbarclick,     {.f = -0.05 } },
	{ ClkWinTitle,          0,              Button5,        ttbarclick,     {.f = +0.05 } },
	//{ ClkWinTitle,          0,              Button4,        setmfact,       {.f = -0.05 } },
	//{ ClkWinTitle,          0,              Button5,        setmfact,       {.f = +0.05 } },
	{ ClkStatusText,        0,              Button4,        spawn,           SHCMD("/home/tinou/hack/scripts/Volume.sh up") },
	{ ClkStatusText,        0,              Button5,        spawn,           SHCMD("/home/tinou/hack/scripts/Volume.sh down") },
	//{ ClkStatusText,        0,              Button1,        spawn,           SHCMD("/home/tinou/bin/inc -w 3 /tmp/isvol && pkill dwm_sleep") },
	{ ClkStatusText,        0,              Button1,        showmpdtoggle,           {.ui = 1 << 5} },
	{ ClkStatusText,        0,              Button2,        spawn,           SHCMD("/home/tinou/hack/scripts/Volume.sh mute") },
	{ ClkStatusText,        0,              Button3,        spawn,           SHCMD("rm /tmp/isvol && pkill dwm_sleep") },
	//{ ClkWinTitle,          0,              Button2,        togglebar,      {0} },
	{ ClkStatusText,        MODKEY,         Button3,        spawn,          SHCMD("pkill gqmpc || LC_ALL=en_US.UTF-8 gqmpc") },
	{ ClkClientWin,         MODKEY,         Button1,        movemouse,      {0} },
	{ ClkClientWin,         MODKEY,         Button2,        togglefloating, {0} },
	{ ClkClientWin,         MODKEY,         Button3,        resizemouse,    {0} },
	{ ClkTagBar,            0,              Button1,        view,           {0} },
	{ ClkTagBar,            0,              Button3,        toggleview,     {0} },
	{ ClkTagBar,            MODKEY,         Button1,        tag,            {0} },
	{ ClkTagBar,            MODKEY,         Button3,        toggletag,      {0} },
};

/* custom functions */
static void
ttbarclick(const Arg *arg) {
	if (lt[sellt]->arrange == &monocle && arg)
		focusstack(arg);
	else
		setmfact(arg);
}

static void
kbmvresize(const Arg *arg) {
	if(!sel) // || (lt[sellt]->arrange) && !sel->isfloating)) -> move only floatings ?
		return;
	if (!sel->isfloating)
		togglefloating(NULL);
	resize(sel, sel->x + ((int *)arg->v)[0],
			sel->y + ((int *)arg->v)[1],
			sel->w + ((int *)arg->v)[2],
			sel->h + ((int *)arg->v)[3], True);
}

static void
rhtoggle(const Arg *arg) {
	resizehints[curtag] ^= 1;
	arrange();
}

static void
updatewindow(const Arg *arg)
{
	updategeom();
	updatebar();
	arrange();
}

int
countvisible(Client *c)
{
	if (c && ISVISIBLE(c))
		return 1 + countvisible(c->next);
	if (c)
		return 0 + countvisible(c->next);
	return 0;
}

static void
showmpdtoggle(const Arg *arg)
{
	const Arg sharg = SHCMD("/home/tinou/bin/inc -w 3 /tmp/isvol && pkill dwm_sleep");
	unsigned int n, m;

	n = countvisible(clients);
	toggleview(arg);
	m = countvisible(clients);
	if (n == m)
		spawn(&sharg);
}
