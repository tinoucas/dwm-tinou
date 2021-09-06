static const Remap mpv[] = {
	{ Button2, ClkClientWin, 0,  XK_q,     0 },
	{ Button11, ClkClientWin, 0,  XK_k, 0},
	{ Button10, ClkClientWin, 0,  XK_j, 0},
	{ Button9, ClkClientWin, 0, XK_k, 0},
	{ Button8, ClkClientWin, 0, XK_j, 0},
	{ 0 },
};

static const Remap sView[] = {
	{ Button9, ClkClientWin, 0, XK_Page_Up, 0},
	{ Button8, ClkClientWin, 0, XK_Page_Down, 0},
	{ Button11, ClkClientWin, 0, XK_Page_Up, 0},
	{ Button10, ClkClientWin, 0, XK_Page_Down, 0},
	{ Button2, ClkClientWin, 0,  XK_Escape, 0 },
	{ Button4, ClkClientWin, 0,  XK_Left,   0 },
	{ Button5, ClkClientWin, 0,  XK_Right,  0 },
	{ 0 },
};

static const Remap chrome[] = {
	{ Button11, ClkClientWin, 0,  XK_k, 0},
	{ Button10, ClkClientWin, 0,  XK_j, 0},
	{ Button9, ClkClientWin, 0, XK_k, 0},
	{ Button8, ClkClientWin, 0, XK_j, 0},
	{ Button12, ClkClientWin, 0, XK_r, 0},
	{ Button1, ClkWinTitle, 0, XK_Home, 0 },
	{ Button2, ClkWinTitle, 0, XK_w, ControlMask },
	{ Button4, ClkWinTitle, 0, XK_Tab, ShiftMask|ControlMask },
	{ Button5, ClkWinTitle, 0, XK_Tab, ControlMask },
	{ Button9, ClkStatusText, 0, XK_Left, 0},
	{ Button8, ClkStatusText, 0, XK_Right, 0},
	{ 0 },
};

static const Remap plex[] = {
	{ Button9, ClkClientWin, 0, XK_Left, ShiftMask},
	{ Button8, ClkClientWin, 0, XK_Right, ShiftMask},
	{ Button11, ClkClientWin, 0, XK_Left, ShiftMask},
	{ Button10, ClkClientWin, 0, XK_Right, ShiftMask},
	{ Button9, ClkStatusText, 0, XK_Left, 0},
	{ Button8, ClkStatusText, 0, XK_Right, 0},
	{ 0 },
};

#define REMAP(a) { #a, a }

const struct {
	const char *name;
	const Remap *remap;
}
remaps[] = {
	REMAP(mpv),
	REMAP(sView),
	REMAP(chrome),
	REMAP(plex),
};
