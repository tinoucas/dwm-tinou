static void
rotateclients (const Arg *arg) {
	Client *base = selmon->clients;
	Client **pc;
	Client *c;
	Client *loopbase = base;

	do
	{
		if (base) {
			if (selmon->lt[selmon->sellt]->arrange != &monocle) {
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
togglemonocle(const Arg *arg) {
	/*static void (*arrange)(Monitor *) = NULL;*/
	static Layout* layout;
	Arg layoutarg;

	if (selmon->lt[selmon->sellt]->arrange != &monocle) {
		/*arrange = selmon->lt[selmon->sellt]->arrange;*/
		layout = (Layout*)selmon->lt[selmon->sellt];
		layoutarg.v = &layouts[MONOCLE];
		setlayout(&layoutarg);
	}
	else {
		layoutarg.v = layout;
		setlayout(&layoutarg);
	}
}

static void
ttbarclick(const Arg *arg) {
	if (selmon->lt[selmon->sellt]->arrange == &monocle && arg)
		focusstack(arg);
	else
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

static void
toggleviews(const Arg *arg) {
	toggleview(arg);
	cleartags(selmon);
	drawbar(selmon);
}

void
allnonfloat(const Arg *arg) {
	Client* c;

	for(c = selmon->clients; c; c = c->next)
		c->isfloating = False;
	arrange(selmon);
}
