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
	/*{ Button9, 0, XK_w, 0},*/
	/*{ Button8, 0, XK_b, 0},*/
	{ Button12, 0, XK_w, ControlMask},
	{ 0 },
};

/*
 *static const Remap torchlight[] = {
 *    { Button9, 0, XK_w },
 *    { Button8, 0, XK_1 },
 *    { Button10, 0, XK_2 },
 *    { Button11, 0, XK_3 },
 *    { Button12, 0, XK_Tab },
 *    { 0, 0, 0}
 *};
 */
