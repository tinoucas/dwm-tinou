static const Remap mpv[] = {
	/*{ Button1, 0,  XK_space, 0 },*/
	{ Button2, 0,  XK_q,     0 },
	/*{ Button3, 0,  XK_f,     0 },*/
	{ Button11, 0,  XK_e,     0 },
	{ Button10, 0,  XK_w,     0 },
	{ Button9, 0, XK_a,     ShiftMask },
	{ 0 },
};

static const Remap sView[] = {
	{ Button11, XK_k, XK_Page_Up, 0},
	{ Button10, XK_j, XK_Page_Down, 0},
	{ Button2, 0,  XK_Escape, 0 },
	{ Button4, 0,  XK_Left,   0 },
	{ Button5, 0,  XK_Right,  0 },
	{ 0 },
};

static const Remap chrome[] = {
	{ Button11, 0,  XK_k, 0},
	{ Button10, 0,  XK_j, 0},
	{ Button9, 0, XK_k, 0},
	{ Button8, 0, XK_j, 0},
	{ Button12, 0, XK_r, 0},
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
};
