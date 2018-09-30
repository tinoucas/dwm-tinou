XKeyEvent makeKeyEvent(Display *display, Window win, Window winRoot, Bool press, KeyCode keycode, int modifiers)
{
	XKeyEvent event = {
		.display     = display,
		.window      = win,
		.root        = winRoot,
		.subwindow   = None,
		.time        = CurrentTime,
		.x           = 1,
		.y           = 1,
		.x_root      = 1,
		.y_root      = 1,
		.same_screen = True,
		.keycode     = keycode,
		.state       = modifiers,
		.type        = press ? KeyPress : KeyRelease
	};
	return event;
}

void sendMotion(int x, int y)
{
    XWarpPointer(dpy, None, root, 0, 0, 0, 0, x, y);
    XFlush(dpy);
}

void centerMouseInWindow (Client* c)
{
	if (c) {
		int x = c->w / 2 + c->x;
		int y = c->h / 2 + c->y;

		sendMotion(x, y);
	}
}

void centerMouseInMonitor (Monitor* m)
{
    if (m) {
        int x = m->mx + m->mw / 2;
        int y = m->my + m->mh / 2;

        sendMotion(x, y);
    }
}

void centerMouseInMonitorIndex (int monitor) {
    Monitor* m;

    for(m = mons; m && m->num != monitor; m = m->next) ;

    if (m)
        centerMouseInMonitor(m);
}

void sendKey(KeyCode keycode, int modifiers)
{
	if (!selmon || !selmon->sel || !selmon->sel->win)
		return ;
	XKeyEvent kep = makeKeyEvent(dpy, selmon->sel->win, root, True, keycode, modifiers);
	XSendEvent(dpy, selmon->sel->win, True, KeyPressMask, (XEvent*)&kep);
	XKeyEvent ker = makeKeyEvent(dpy, selmon->sel->win, root, False, keycode, modifiers);
	XSendEvent(dpy, selmon->sel->win, True, KeyPressMask, (XEvent*)&ker);
	XUngrabKeyboard(dpy, CurrentTime);
}

static void sendbracketright(const Arg *arg)
{
	sendKey(XKeysymToKeycode(dpy, XK_bracketright), ControlMask|Mod5Mask);
}

static void sendbackslash(const Arg *arg)
{
	sendKey(XKeysymToKeycode(dpy, XK_backslash), ControlMask|Mod5Mask);
}

static void sendbracketleft(const Arg *arg)
{
	sendKey(XKeysymToKeycode(dpy, XK_bracketleft), ControlMask|Mod5Mask);
}

#if 0
static void sendkey(const Arg *arg)
{
	sendKey(XKeysymToKeycode(dpy, arg->ui), 0);
}
#endif
