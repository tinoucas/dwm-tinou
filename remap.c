#include "remap.h"

XKeyEvent
makeKeyEvent(Display *display, Window win, Window winRoot, Bool press, KeyCode keycode, int modifiers) {
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

void
sendKey(KeyCode keycode, int modifiers) {
	if (!selmon || !selmon->sel || !selmon->sel->win)
		return ;
	XKeyEvent kep = makeKeyEvent(dpy, selmon->sel->win, root, True, keycode, modifiers);
	XSendEvent(dpy, selmon->sel->win, True, KeyPressMask, (XEvent*)&kep);
	XKeyEvent ker = makeKeyEvent(dpy, selmon->sel->win, root, False, keycode, modifiers);
	XSendEvent(dpy, selmon->sel->win, True, KeyPressMask, (XEvent*)&ker);
	XUngrabKeyboard(dpy, CurrentTime);
}
