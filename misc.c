void
rotateclients (const Arg *arg) {
	Client *base = selmon->clients;
	Client **pc;
	Client *c;
	Client *loopbase = base;

	do
	{
		if (base) {
			if (selmon->vs->lt[selmon->vs->curlt]->arrange != &monocle) {
				if (arg->i > 0) {
					selmon->clients = base->next;
					pc = &selmon->clients;
					while (*pc)
						pc = &(*pc)->next;
					*pc = base;
					base->next = NULL;
				}
				else {
					c = selmon->clients;
					while (c->next)
						c = c->next;
					detach(c);
					attach(c);
				}
			}
			else {
				focusstack(arg);
			}
		}
		base = selmon->clients;
		if (base == loopbase)
			break;
	} while (base && (!ISVISIBLE(base) || base->nofocus));
	arrange(selmon);
}

static void
ttbarclick(const Arg *arg) {
		focusstack(arg);
}

static void
viewscroll(const Arg *arg) {
	if (selmon->vs->lt[selmon->vs->curlt]->arrange != &monocle)
		setmfact(arg);
}

static void
opacitychange(const Arg *arg) {
	if (selmon->sel)
	{
		double opacity = selmon->sel->opacity;
		selmon->sel->opacity = MAX(0, MIN(1, opacity + arg->f));
		client_opacity_set(selmon->sel, selmon->sel->opacity);
	}
}

static void
focuslast(const Arg *arg) {
	if (lastclient)
		focus(lastclient);
}

void
allnonfloat(const Arg *arg) {
	Client* c;

	for(c = selmon->clients; c; c = c->next)
		c->isfloating = False;
	arrange(selmon);
}

void
sendMotion(int x, int y) {
    XWarpPointer(dpy, None, root, 0, 0, 0, 0, x, y);
    XFlush(dpy);
}

void
centerMouseInWindow(Client* c) {
	if (c) {
		int x = c->w / 2 + c->x;
		int y = c->h / 2 + c->y;

		sendMotion(x, y);
	}
}

void
centerMouseInMonitor(Monitor* m) {
    if (m) {
        int x = m->mx + m->mw / 2;
        int y = m->my + m->mh / 2;

        sendMotion(x, y);
    }
}

void
centerMouseInMonitorIndex(int monitor) {
    Monitor* m;

    for(m = mons; m && m->num != monitor; m = m->next) ;

    if (m)
        centerMouseInMonitor(m);
}

